"""Guarded 500 Hz Willow multisine acquisition with host gravity feedforward."""
from __future__ import annotations
import csv, hashlib, os, time
from pathlib import Path
import numpy as np
import pinocchio as pin
import pyflorid

ROOT = Path(__file__).resolve().parent
_LOCAL_URDF = ROOT.parent / "static_gravity_calibration/model/identified/Ragtime_Willow.static-mass-com-calibrated.urdf"
URDF = _LOCAL_URDF if _LOCAL_URDF.exists() else ROOT.parents[1] / "assets/urdf/Ragtime_Willow_static_mass_com_calibrated.urdf"
_LOCAL_DESIGN = ROOT / "dynamic_excitation_design.npz"
DESIGN = _LOCAL_DESIGN if _LOCAL_DESIGN.exists() else ROOT.parents[1] / "data/dynamic_excitation_design.npz"
RUNS = ROOT / "runs"
RUN_LABEL = "identification"
DEVICE_URI = "usb:///dev/ttyACM0"
ENABLE_HARDWARE = False
CONFIRMATION_PHRASE = "RUN WILLOW DYNAMIC IDENTIFICATION"
KP = np.array([30., 60., 60., 50., 40., 18.], dtype=np.float32)
KD = np.array([1.6, 2.0, 1.7, 1.4, 1.1, 1.2], dtype=np.float32)
MOVE_SPEED_DEG_S = 25.0
STATE_TIMEOUT_S = .08
MAX_INTERFRAME_S = .020
MIN_TRANSITION_CLEARANCE_M = .005
OBSERVED_J2_PLATEAU_NM = 11.9
MAX_CONSECUTIVE_J2_PLATEAU_FRAMES = 10
MIN_FEEDBACK_RATE_HZ = 300.0
RATE_WINDOW_S = 1.0
MAX_FOLLOWING_ERROR_DEG = np.array([15., 15., 15., 18., 18., 20.])
LOWER_DEG = np.array([-170., 5., 5., -70., -85., -85.])
UPPER_DEG = np.array([170., 175., 175., 70., 85., 85.])
TAU_LIMIT_NM = np.array([10., 28., 28., 10., 10., 10.])
COLUMNS = ("time_s", "host_time_s", "device_time", "seq", "errors", "design_sha256", "urdf_sha256",
    *(f"target_q{i}_rad" for i in range(1,7)), *(f"target_dq{i}_rad_s" for i in range(1,7)),
    *(f"target_ddq{i}_rad_s2" for i in range(1,7)), *(f"q{i}_rad" for i in range(1,7)),
    *(f"dq{i}_rad_s" for i in range(1,7)), *(f"tau{i}_nm" for i in range(1,7)),
    *(f"gravity{i}_nm" for i in range(1,7)), *(f"kp{i}" for i in range(1,7)), *(f"kd{i}" for i in range(1,7)))

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
    if np.any(q_deg<=LOWER_DEG) or np.any(q_deg>=UPPER_DEG):raise RuntimeError(f"measured joint crossed software limit: {np.round(q_deg,2)}")
    g=gravity(model,data,q);control.write_once(command(q_ref,dq_ref,g));return state,q,g

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
    duration=max(.5,float(np.max(np.abs(target-start)))/np.deg2rad(MOVE_SPEED_DEG_S));begin=time.monotonic()
    while True:
        alpha=min(1.,(time.monotonic()-begin)/duration);desired=start+alpha*(target-start)
        state,_,_=cycle(control,model,data,desired,np.zeros(6),last_seq);last_seq=int(state.seq)
        if alpha>=1.:break
    until=time.monotonic()+1.
    while time.monotonic()<until:
        state,_,_=cycle(control,model,data,target,np.zeros(6),last_seq);last_seq=int(state.seq)

def row(state,q,g,q_ref,dq_ref,ddq_ref,elapsed,design_sha256,urdf_sha256):
    source_timestamp_us=int(getattr(state,"source_timestamp_us",0))
    device_time_s=source_timestamp_us*1e-6 if source_timestamp_us>0 else float(state.time)*1e-3
    result={"time_s":elapsed,"host_time_s":time.time(),"device_time":device_time_s,"seq":int(state.seq),"errors":int(state.errors),
        "design_sha256":design_sha256,"urdf_sha256":urdf_sha256}
    for prefix,values,suffix in (("target_q",q_ref,"_rad"),("target_dq",dq_ref,"_rad_s"),("target_ddq",ddq_ref,"_rad_s2"),
        ("q",q,"_rad"),("dq",state.dq,"_rad_s"),("tau",state.tau,"_nm"),("gravity",g,"_nm"),("kp",KP,""),("kd",KD,"")):
        result.update({f"{prefix}{i}{suffix}":float(v) for i,v in enumerate(values,1)})
    return result

def main():
    design=np.load(DESIGN);t,q_ref,dq_ref,ddq_ref=(np.asarray(design[x]) for x in ("t","q","dq","ddq"))
    if (t.ndim!=1 or len(t)<2 or np.any(np.diff(t)<=0) or any(x.shape!=(len(t),6) for x in (q_ref,dq_ref,ddq_ref))
            or not np.all(np.isfinite(np.c_[t,q_ref,dq_ref,ddq_ref]))):raise ValueError("malformed/non-finite dynamic design")
    model=pin.buildModelFromUrdf(str(URDF));data=model.createData();RUNS.mkdir(parents=True,exist_ok=True)
    nominal_tau=np.asarray([pin.rnea(model,data,qi,dqi,ai) for qi,dqi,ai in zip(q_ref,dq_ref,ddq_ref)])
    urdf_sha256=hashlib.sha256(URDF.read_bytes()).hexdigest();design_sha256=hashlib.sha256(DESIGN.read_bytes()).hexdigest()
    print("URDF SHA256:",urdf_sha256);print("design:",DESIGN,"SHA256:",design_sha256,"samples=",len(t),"rate=",1/np.median(np.diff(t)))
    print("run label:", RUN_LABEL)
    print("nominal inverse-dynamics peak Nm:",np.round(np.max(np.abs(nominal_tau),axis=0),3))
    if np.max(np.abs(nominal_tau[:,1])) >= OBSERVED_J2_PLATEAU_NM:
        raise RuntimeError("design predicts J2 torque at/above the observed 11.9 Nm plateau")
    print("MIT host gravity only; firmware_gravity=False; output uses measured q/dq/tau; DISABLED preview:",not ENABLE_HARDWARE)
    if not ENABLE_HARDWARE:return
    arm=pyflorid.Arm.create(DEVICE_URI)
    if arm is None:raise RuntimeError(f"Arm.create failed for {DEVICE_URI}")
    initial=read_valid(arm,2.);offset=float(np.asarray(initial.q)[0]);q_ref=q_ref.copy();q_ref[:,0]+=offset
    q_ref_deg=np.rad2deg(q_ref)
    if np.any(q_ref_deg<=LOWER_DEG) or np.any(q_ref_deg>=UPPER_DEG):
        raise RuntimeError(f"measured J1 offset moves the design outside software limits; q1 range={q_ref_deg[:,0].min():.2f}..{q_ref_deg[:,0].max():.2f}deg")
    print("startup q_deg:",np.round(np.rad2deg(np.asarray(initial.q)),3));print("J1 design offset deg:",np.rad2deg(offset))
    print("actual start-transition collision audit:",audit_start_transition(model,URDF,np.asarray(initial.q,dtype=float),q_ref[0]))
    if input(f'Type exactly "{CONFIRMATION_PHRASE}" while ready to stop the arm: ')!=CONFIRMATION_PHRASE:raise RuntimeError("confirmation mismatch")
    stamp=time.strftime("%Y%m%dT%H%M%S");final=RUNS/f"dynamic_{RUN_LABEL}_{stamp}.csv";temp=final.with_suffix(".csv.tmp")
    try:
        arm.enable();control=arm.start_joint_mit_control()
        if control is None:raise RuntimeError("start_joint_mit_control returned None")
        move(control,model,data,q_ref[0]);begin=time.monotonic();index=0;j2_plateau_frames=0;last_seq=None;last_feedback_time=None
        rate_begin=begin;rate_frames=0
        with temp.open("w",newline="",encoding="utf-8") as file:
            writer=csv.DictWriter(file,fieldnames=COLUMNS);writer.writeheader()
            while index<len(t):
                elapsed=time.monotonic()-begin
                desired=min(int(elapsed/np.median(np.diff(t))),len(t)-1)
                state,q,g=cycle(control,model,data,q_ref[desired],dq_ref[desired],last_seq);last_seq=int(state.seq)
                feedback_time=time.monotonic()
                if last_feedback_time is not None and feedback_time-last_feedback_time>MAX_INTERFRAME_S:
                    raise RuntimeError(f"feedback gap {feedback_time-last_feedback_time:.4f}s exceeds {MAX_INTERFRAME_S:.4f}s")
                last_feedback_time=feedback_time
                error_deg=np.abs(np.rad2deg(q_ref[desired]-q))
                if np.any(error_deg>MAX_FOLLOWING_ERROR_DEG):raise RuntimeError(f"following error exceeded: {np.round(error_deg,2)} deg")
                j2_plateau_frames = j2_plateau_frames + 1 if abs(float(state.tau[1])) >= OBSERVED_J2_PLATEAU_NM else 0
                if j2_plateau_frames >= MAX_CONSECUTIVE_J2_PLATEAU_FRAMES:
                    raise RuntimeError("J2 feedback torque remained on the observed 11.9 Nm plateau")
                writer.writerow(row(state,q,g,q_ref[desired],dq_ref[desired],ddq_ref[desired],elapsed,design_sha256,urdf_sha256));index=desired+1;rate_frames+=1
                now=time.monotonic()
                if now-rate_begin>=RATE_WINDOW_S:
                    rate_hz=rate_frames/(now-rate_begin);print(f"feedback/control rate={rate_hz:.1f} Hz, t={elapsed:.2f}s, max error={np.max(error_deg):.2f}deg")
                    if rate_hz<MIN_FEEDBACK_RATE_HZ:raise RuntimeError(f"feedback/control rate {rate_hz:.1f} Hz below {MIN_FEEDBACK_RATE_HZ:g} Hz")
                    rate_begin=now;rate_frames=0
            file.flush();os.fsync(file.fileno())
        temp.replace(final)
        if hasattr(os,"O_DIRECTORY"):
            directory_fd=os.open(RUNS,os.O_RDONLY|os.O_DIRECTORY)
            try:os.fsync(directory_fd)
            finally:os.close(directory_fd)
        print("saved+fsynced:",final)
    finally:
        arm.disable();print("All axes disabled.")

if __name__=="__main__":main()
