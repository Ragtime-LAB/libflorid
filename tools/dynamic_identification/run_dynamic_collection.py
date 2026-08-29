"""Guarded 500 Hz Willow multisine acquisition with host gravity feedforward."""
from __future__ import annotations
import csv, hashlib, os, sys, time
from pathlib import Path
import numpy as np
import pinocchio as pin
import pyflorid

ROOT = Path(__file__).resolve().parent
_LOCAL_URDF = ROOT.parent / "static_gravity_calibration/model/identified/Ragtime_Willow.static-mass-com-calibrated.urdf"
URDF = _LOCAL_URDF if _LOCAL_URDF.exists() else ROOT.parents[1] / "assets/urdf/Ragtime_Willow_static_mass_com_calibrated.urdf"
_LOCAL_DESIGN = ROOT / "dynamic_two_pose_excitation_design.npz"
DESIGN = _LOCAL_DESIGN if _LOCAL_DESIGN.exists() else ROOT.parents[1] / "data/dynamic_excitation_design.npz"
RUNS = ROOT / "runs"
RUN_LABEL = "two_pose_identification"
DEVICE_URI = "usb:///dev/ttyACM0"
ENABLE_HARDWARE = True
CONFIRMATION_PHRASE = "RUN WILLOW DYNAMIC IDENTIFICATION"
KP = np.array([30., 60., 60., 50., 40., 18.], dtype=np.float32)
KD = np.array([1.6, 2.0, 1.7, 1.4, 1.1, 1.2], dtype=np.float32)
MOVE_SPEED_DEG_S = 90.0
MOVE_MAX_ACCEL_DEG_S2 = 30.0
TAKEOVER_HOLD_S = 0.5
STATE_TIMEOUT_S = .08
MAX_INTERFRAME_S = .020
MIN_TRANSITION_CLEARANCE_M = .005
MIN_FEEDBACK_RATE_HZ = 300.0
RATE_WINDOW_S = 1.0
MAX_FOLLOWING_ERROR_DEG = np.array([15., 15., 15., 18., 18., 20.])
LOWER_DEG = np.array([-170., -180., -180., -180., -180., -180.])
UPPER_DEG = np.array([170., 180., 180., 180., 180., 180.])
TAU_LIMIT_NM = np.array([40., 40., 40., 12., 12., 12.])
APPLY_MEASURED_J1_OFFSET = False
ENFORCE_SOFTWARE_LIMITS = False
ENFORCE_START_COLLISION_AUDIT = False
RETURN_TO_START_AFTER_RUN = True
MAX_RUN_ATTEMPTS = 3

# Reuse the already hand-tested gravity/friction implementation.  J1 and J2/J3
# use the same actuator module, so J1 receives a copy of the J2 dynamic model;
# it is evaluated with J1's own measured velocity.
sys.path.insert(0, str(ROOT.parent / "friction_calibration"))
import run_gravity_friction_feedforward as friction_ff
FRICTION_MODELS = None
BREAKAWAY_MODELS = None
COLUMNS = ("time_s", "host_time_s", "device_time", "seq", "errors", "design_sha256", "urdf_sha256",
    *(f"target_q{i}_rad" for i in range(1,7)), *(f"target_dq{i}_rad_s" for i in range(1,7)),
    *(f"target_ddq{i}_rad_s2" for i in range(1,7)), *(f"q{i}_rad" for i in range(1,7)),
    *(f"dq{i}_rad_s" for i in range(1,7)), *(f"tau{i}_nm" for i in range(1,7)),
    *(f"gravity{i}_nm" for i in range(1,7)), *(f"friction{i}_nm" for i in range(1,7)),
    *(f"breakaway{i}_nm" for i in range(1,7)), *(f"feedforward{i}_nm" for i in range(1,7)),
    *(f"kp{i}" for i in range(1,7)), *(f"kd{i}" for i in range(1,7)))

def read_valid(reader, timeout=STATE_TIMEOUT_S, last_seq=None):
    deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        state=reader.read_once()
        if int(state.seq)!=0 and (last_seq is None or int(state.seq)!=int(last_seq)):return state
        time.sleep(.0002)
    raise TimeoutError("no valid ArmState")

def gravity(model,data,q):
    g=np.asarray(pin.computeGeneralizedGravity(model,data,q),dtype=float);g[0]=0.
    if g.shape!=(6,) or not np.all(np.isfinite(g)) or np.any(np.abs(g)>TAU_LIMIT_NM):
        raise RuntimeError(f"gravity command invalid or outside effective MIT range: {g}")
    return g

def command(q,dq,tau):
    cmd=pyflorid.JointMIT();cmd.q=np.asarray(q,dtype=np.float32);cmd.dq=np.asarray(dq,dtype=np.float32)
    cmd.tau=np.asarray(tau,dtype=np.float32);cmd.kp=KP;cmd.kd=KD;cmd.firmware_gravity=False;return cmd

def cycle(control,model,data,q_ref,dq_ref,last_seq=None):
    state=read_valid(control,last_seq=last_seq);errors=int(state.errors)
    if errors:raise RuntimeError(f"firmware errors=0x{errors:08X}")
    q=np.asarray(state.q,dtype=float);dq=np.asarray(state.dq,dtype=float);tau=np.asarray(state.tau,dtype=float)
    q_ref=np.asarray(q_ref,dtype=float);dq_ref=np.asarray(dq_ref,dtype=float)
    if any(value.shape!=(6,) for value in (q,dq,tau,q_ref,dq_ref)) or not np.all(np.isfinite(np.r_[q,dq,tau,q_ref,dq_ref])):
        raise RuntimeError("non-finite or malformed state/reference")
    q_deg=np.rad2deg(q)
    if ENFORCE_SOFTWARE_LIMITS and (np.any(q_deg<=LOWER_DEG) or np.any(q_deg>=UPPER_DEG)):
        raise RuntimeError(f"measured joint crossed software limit: {np.round(q_deg,2)}")
    g=gravity(model,data,q)
    friction=friction_ff.friction_torque(dq,FRICTION_MODELS)
    breakaway=friction_ff.breakaway_torque(dq,g,FRICTION_MODELS,BREAKAWAY_MODELS)
    feedforward=g+friction+breakaway
    if np.any(np.abs(feedforward)>TAU_LIMIT_NM):
        raise RuntimeError(f"feedforward exceeds configured motor limits: {feedforward}")
    control.write_once(command(q_ref,dq_ref,feedforward));return state,q,g,friction,breakaway,feedforward

def audit_start_transition(model,urdf_path,start,target):
    steps=max(2,int(np.ceil(np.max(np.abs(np.rad2deg(target-start)))))+1)
    geometry=pin.buildGeomFromUrdf(model,str(urdf_path),pin.GeometryType.COLLISION,[str(Path(urdf_path).parent)])
    geometry.addAllCollisionPairs()
    if not geometry.geometryObjects or not geometry.collisionPairs:raise RuntimeError("URDF lacks collision geometry")
    md=model.createData();gd=pin.GeometryData(geometry);minimum=float("inf");closest=None
    for sample,q in enumerate(np.linspace(start,target,steps)):
        q_deg=np.rad2deg(q)
        if np.any(q_deg<=LOWER_DEG) or np.any(q_deg>=UPPER_DEG):raise RuntimeError("start transition crosses software limit")
        pin.computeDistances(model,md,geometry,gd,q)
        for i,pair in enumerate(geometry.collisionPairs):
            first=geometry.geometryObjects[pair.first];second=geometry.geometryObjects[pair.second]
            if abs(int(first.parentJoint)-int(second.parentJoint))<=1:continue
            distance=float(gd.distanceResults[i].min_distance)
            if distance<minimum:minimum=distance;closest=(sample,first.name,second.name)
    if not np.isfinite(minimum) or minimum<MIN_TRANSITION_CLEARANCE_M:
        raise RuntimeError(f"start transition clearance {minimum:.6g}m below {MIN_TRANSITION_CLEARANCE_M:g}m; closest={closest}")
    return {"samples":steps,"minimum_distance_m":minimum,"closest":closest}

def move(control,model,data,target):
    state=read_valid(control);last_seq=int(state.seq);start=np.asarray(state.q,dtype=float)
    # First command the newly measured pose verbatim, so entering MIT cannot
    # create a position step.  Then use a quintic minimum-jerk interpolation;
    # its peak normalized speed is 1.875, hence the duration scaling below.
    transition_begin=time.monotonic();frames=0
    hold_until=time.monotonic()+TAKEOVER_HOLD_S
    while time.monotonic()<hold_until:
        state,*_=cycle(control,model,data,start,np.zeros(6),last_seq);last_seq=int(state.seq);frames+=1
    distance=float(np.max(np.abs(target-start)))
    velocity_duration=1.875*distance/np.deg2rad(MOVE_SPEED_DEG_S)
    acceleration_duration=np.sqrt(5.8*distance/np.deg2rad(MOVE_MAX_ACCEL_DEG_S2))
    duration=max(1.0,velocity_duration,acceleration_duration);begin=time.monotonic()
    while True:
        u=min(1.,(time.monotonic()-begin)/duration)
        alpha=10.*u**3-15.*u**4+6.*u**5
        alpha_dot=(30.*u**2-60.*u**3+30.*u**4)/duration
        desired=start+alpha*(target-start);desired_dq=alpha_dot*(target-start)
        state,*_=cycle(control,model,data,desired,desired_dq,last_seq);last_seq=int(state.seq);frames+=1
        if u>=1.:break
    until=time.monotonic()+1.
    while time.monotonic()<until:
        state,*_=cycle(control,model,data,target,np.zeros(6),last_seq);last_seq=int(state.seq);frames+=1
    transition_elapsed=time.monotonic()-transition_begin
    final_q=np.asarray(state.q,dtype=float)
    print(f"transition rate={frames/transition_elapsed:.1f} Hz, duration={transition_elapsed:.2f}s, "
          f"final max error={np.max(np.abs(np.rad2deg(target-final_q))):.2f}deg")

def row(state,q,g,friction,breakaway,feedforward,q_ref,dq_ref,ddq_ref,elapsed,design_sha256,urdf_sha256):
    source_timestamp_us=int(getattr(state,"source_timestamp_us",0))
    device_time_s=source_timestamp_us*1e-6 if source_timestamp_us>0 else float(state.time)*1e-3
    result={"time_s":elapsed,"host_time_s":time.time(),"device_time":device_time_s,"seq":int(state.seq),"errors":int(state.errors),
        "design_sha256":design_sha256,"urdf_sha256":urdf_sha256}
    for prefix,values,suffix in (("target_q",q_ref,"_rad"),("target_dq",dq_ref,"_rad_s"),("target_ddq",ddq_ref,"_rad_s2"),
        ("q",q,"_rad"),("dq",state.dq,"_rad_s"),("tau",state.tau,"_nm"),("gravity",g,"_nm"),
        ("friction",friction,"_nm"),("breakaway",breakaway,"_nm"),("feedforward",feedforward,"_nm"),
        ("kp",KP,""),("kd",KD,"")):
        result.update({f"{prefix}{i}{suffix}":float(v) for i,v in enumerate(values,1)})
    return result

def acquire_once(control,model,data,t,q_ref,dq_ref,ddq_ref,temp,design_sha256,urdf_sha256):
    move(control,model,data,q_ref[0]);begin=time.monotonic();index=0;last_seq=None;last_feedback_time=None
    rate_begin=begin;rate_frames=0;sample_period=float(np.median(np.diff(t)))
    with temp.open("w",newline="",encoding="utf-8") as file:
        writer=csv.DictWriter(file,fieldnames=COLUMNS);writer.writeheader()
        while index<len(t):
            elapsed=time.monotonic()-begin
            desired=min(int(elapsed/sample_period),len(t)-1)
            state,q,g,friction,breakaway,feedforward=cycle(
                control,model,data,q_ref[desired],dq_ref[desired],last_seq)
            last_seq=int(state.seq);feedback_time=time.monotonic()
            if last_feedback_time is not None and feedback_time-last_feedback_time>MAX_INTERFRAME_S:
                raise RuntimeError(f"feedback gap {feedback_time-last_feedback_time:.4f}s exceeds {MAX_INTERFRAME_S:.4f}s")
            last_feedback_time=feedback_time
            error_deg=np.abs(np.rad2deg(q_ref[desired]-q))
            if np.any(error_deg>MAX_FOLLOWING_ERROR_DEG):
                raise RuntimeError(f"following error exceeded: {np.round(error_deg,2)} deg")
            writer.writerow(row(state,q,g,friction,breakaway,feedforward,q_ref[desired],
                dq_ref[desired],ddq_ref[desired],elapsed,design_sha256,urdf_sha256))
            index=desired+1;rate_frames+=1;now=time.monotonic()
            if now-rate_begin>=RATE_WINDOW_S:
                rate_hz=rate_frames/(now-rate_begin)
                print(f"feedback/control rate={rate_hz:.1f} Hz, t={elapsed:.2f}s, max error={np.max(error_deg):.2f}deg")
                if rate_hz<MIN_FEEDBACK_RATE_HZ:
                    raise RuntimeError(f"feedback/control rate {rate_hz:.1f} Hz below {MIN_FEEDBACK_RATE_HZ:g} Hz")
                rate_begin=now;rate_frames=0
        file.flush();os.fsync(file.fileno())

def recoverable_acquisition_error(error):
    text=str(error).lower()
    return friction_ff.recoverable(error) or any(token in text for token in (
        "following error exceeded", "feedback gap", "feedback/control rate"))

def main():
    global FRICTION_MODELS, BREAKAWAY_MODELS
    design=np.load(DESIGN);t,q_ref,dq_ref,ddq_ref=(np.asarray(design[x]) for x in ("t","q","dq","ddq"))
    if (t.ndim!=1 or len(t)<2 or np.any(np.diff(t)<=0) or any(x.shape!=(len(t),6) for x in (q_ref,dq_ref,ddq_ref))
            or not np.all(np.isfinite(np.c_[t,q_ref,dq_ref,ddq_ref]))):raise ValueError("malformed/non-finite dynamic design")
    model=pin.buildModelFromUrdf(str(URDF));data=model.createData();RUNS.mkdir(parents=True,exist_ok=True)
    FRICTION_MODELS=friction_ff.load_friction_models();FRICTION_MODELS[1]=FRICTION_MODELS[2]
    BREAKAWAY_MODELS=friction_ff.load_breakaway_models();BREAKAWAY_MODELS[1]=BREAKAWAY_MODELS[2]
    friction_ff.FRICTION_SCALE[0]=friction_ff.FRICTION_SCALE[1]
    friction_ff.EXTRA_VISCOUS_NM_PER_RAD_S[0]=friction_ff.EXTRA_VISCOUS_NM_PER_RAD_S[1]
    friction_ff.BREAKAWAY_SCALE[0]=friction_ff.BREAKAWAY_SCALE[1]
    nominal_tau=np.asarray([pin.rnea(model,data,qi,dqi,ai) for qi,dqi,ai in zip(q_ref,dq_ref,ddq_ref)])
    urdf_sha256=hashlib.sha256(URDF.read_bytes()).hexdigest();design_sha256=hashlib.sha256(DESIGN.read_bytes()).hexdigest()
    print("URDF SHA256:",urdf_sha256);print("design:",DESIGN,"SHA256:",design_sha256,"samples=",len(t),"rate=",1/np.median(np.diff(t)))
    print("run label:", RUN_LABEL)
    print("nominal inverse-dynamics peak Nm:",np.round(np.max(np.abs(nominal_tau),axis=0),3))
    print("friction models:",{f"J{joint}":item[0] for joint,item in FRICTION_MODELS.items()})
    print("J1 friction reuses J2 model and scale:",float(friction_ff.FRICTION_SCALE[0]))
    print("MIT host gravity + fitted friction; firmware_gravity=False; output uses measured q/dq/tau; DISABLED preview:",not ENABLE_HARDWARE)
    if not ENABLE_HARDWARE:return
    arm=pyflorid.Arm.create(DEVICE_URI)
    if arm is None:raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
    arm.automatic_error_recovery();time.sleep(1.0)
    initial=read_valid(arm,2.);offset=float(np.asarray(initial.q)[0]);q_ref=q_ref.copy()
    if APPLY_MEASURED_J1_OFFSET:q_ref[:,0]+=offset
    q_ref_deg=np.rad2deg(q_ref)
    if ENFORCE_SOFTWARE_LIMITS and (np.any(q_ref_deg<=LOWER_DEG) or np.any(q_ref_deg>=UPPER_DEG)):
        raise RuntimeError(f"measured J1 offset moves the design outside software limits; q1 range={q_ref_deg[:,0].min():.2f}..{q_ref_deg[:,0].max():.2f}deg")
    print("startup q_deg:",np.round(np.rad2deg(np.asarray(initial.q)),3));print("J1 design offset applied:",APPLY_MEASURED_J1_OFFSET)
    if ENFORCE_START_COLLISION_AUDIT:
        print("actual start-transition collision audit:",audit_start_transition(model,URDF,np.asarray(initial.q,dtype=float),q_ref[0]))
    else:print("start-transition collision audit bypassed by operator physical-workspace confirmation")
    if input(f'Type exactly "{CONFIRMATION_PHRASE}" while ready to stop the arm: ')!=CONFIRMATION_PHRASE:raise RuntimeError("confirmation mismatch")
    stamp=time.strftime("%Y%m%dT%H%M%S");final=RUNS/f"dynamic_{RUN_LABEL}_{stamp}.csv";temp=final.with_suffix(".csv.tmp")
    try:
        control=friction_ff.start_session(arm)
        for attempt in range(1,MAX_RUN_ATTEMPTS+1):
            try:
                print(f"dynamic acquisition attempt {attempt}/{MAX_RUN_ATTEMPTS}")
                acquire_once(control,model,data,t,q_ref,dq_ref,ddq_ref,temp,design_sha256,urdf_sha256)
                break
            except Exception as error:
                if temp.exists():temp.unlink()
                if attempt>=MAX_RUN_ATTEMPTS or not recoverable_acquisition_error(error):raise
                print(f"recoverable acquisition failure: {error}")
                arm,control=friction_ff.recover_session(arm,error)
                print("fault cleared; returning to measured startup pose")
                move(control,model,data,np.asarray(initial.q,dtype=float))
                print("startup pose restored; restarting trajectory from sample zero")
        temp.replace(final)
        if hasattr(os,"O_DIRECTORY"):
            directory_fd=os.open(RUNS,os.O_RDONLY|os.O_DIRECTORY)
            try:os.fsync(directory_fd)
            finally:os.close(directory_fd)
        print("saved+fsynced:",final)
        if RETURN_TO_START_AFTER_RUN:
            print("returning to measured startup pose at",MOVE_SPEED_DEG_S,"deg/s")
            move(control,model,data,np.asarray(initial.q,dtype=float))
            print("startup pose restored")
    finally:
        friction_ff.best_effort_disable(arm);print("All axes disabled.")

if __name__=="__main__":main()
