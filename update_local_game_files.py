from __future__ import annotations

import ctypes
import os
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


# ==========================================================
# CONSOLE LOGGING
# ==========================================================

def log(msg: str) -> None:
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"{ts} {msg}")


def has_flag(flag: str) -> bool:
    flag_l = flag.lower()
    return any(a.lower() == flag_l for a in sys.argv[1:])


def pause() -> None:
    try:
        input("Press ENTER to close...")
    except Exception:
        pass


# ==========================================================
# ADMIN BOOTSTRAP
# ==========================================================

def ensure_admin() -> None:
    if has_flag("--elevated"):
        return

    try:
        is_admin = bool(ctypes.windll.shell32.IsUserAnAdmin())
    except Exception:
        is_admin = False

    if is_admin:
        return

    new_args: List[str] = []
    for a in sys.argv[1:]:
        if a.lower() in ("--elevated",):
            continue
        new_args.append(a)

    new_args.append("--elevated")

    args = " ".join([f'"{a}"' for a in new_args])
    cmdline = f'"{Path(__file__).resolve()}" {args}'.strip()

    log("Not admin. Relaunching elevated...")
    rc = ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, cmdline, None, 1)

    if rc <= 32:
        log("ERROR: Elevation cancelled or failed.")
        raise SystemExit(1)

    raise SystemExit(0)


# ==========================================================
# CONFIG
# ==========================================================

ROOT_MGS2 = Path(r"C:\Vortex\metalgearsolid2mc\mods")
ROOT_MG_MG2 = Path(r"C:\Vortex\metalgearandmetalgear2mc\mods")
ROOT_MGS3 = Path(r"C:\Vortex\metalgearsolid3mc\mods")

ROOTS = [ROOT_MGS2, ROOT_MG_MG2, ROOT_MGS3]

SRC_ASI = Path(r"C:\Development\Git\MGSHDFix\x64\Release\MGSHDFix.asi")
SRC_CFG = Path(r"C:\Development\Git\MGSHDFix\x64\Release\MGSHDFix Config Tool.exe")

# These will be placed under each mod root at: <mod>\plugins\<name>
DEPLOY_LINKS: List[Tuple[Path, str]] = [
    (SRC_ASI, "MGSHDFix.asi"),
    (SRC_CFG, "MGSHDFix Config Tool.exe"),
]


LOG_FILES = [
    "MGSHDFix_Game.log",
    "MGSHDFix_Launcher.log",
]


# ==========================================================
# SYMLINK HELPERS
# ==========================================================

def can_create_symlink_in_dir(dir_path: Path) -> bool:
    probe_target = dir_path / "__mgshdfix_symlink_probe_target.tmp"
    probe_link = dir_path / "__mgshdfix_symlink_probe_link.tmp"

    try:
        dir_path.mkdir(parents=True, exist_ok=True)

        if probe_link.exists() or probe_link.is_symlink():
            probe_link.unlink()
        if probe_target.exists():
            probe_target.unlink()

        probe_target.write_text("probe", encoding="utf-8")
        probe_link.symlink_to(probe_target)

        probe_link.unlink()
        probe_target.unlink()
        return True
    except OSError:
        try:
            if probe_link.exists() or probe_link.is_symlink():
                probe_link.unlink()
            if probe_target.exists():
                probe_target.unlink()
        except OSError:
            pass
        return False


def resolve_symlink_target(path: Path) -> Optional[Path]:
    try:
        if not path.is_symlink():
            return None
        return Path(os.readlink(path)).resolve()
    except OSError:
        return None


def ensure_file_symlink(src_file: Path, dst_file: Path) -> str:
    if not src_file.exists():
        return f"[ERROR] Source missing: {src_file}"

    dst_file.parent.mkdir(parents=True, exist_ok=True)

    existing_target = resolve_symlink_target(dst_file)
    if existing_target is not None and existing_target == src_file.resolve():
        return f"[OK] Link correct: {dst_file}"

    can_link = can_create_symlink_in_dir(dst_file.parent)
    if not can_link:
        return f"[SKIP] No symlink privilege, leaving existing: {dst_file}"

    try:
        if dst_file.exists() or dst_file.is_symlink():
            dst_file.unlink()
        dst_file.symlink_to(src_file)
        return f"[LINK] {dst_file} -> {src_file}"
    except OSError as exc:
        return f"[ERROR] Failed to create symlink: {dst_file} -> {src_file} ({exc})"


def ensure_settings_link_or_copy(src: Path, dst: Path) -> str:
    if not src.exists():
        return f"[ERROR] Source settings missing: {src}"

    dst.parent.mkdir(parents=True, exist_ok=True)

    try:
        if dst.is_symlink():
            current = Path(os.readlink(dst)).resolve()
            if current == src.resolve():
                return f"[OK] Link correct: {dst}"
    except OSError:
        pass

    can_link = can_create_symlink_in_dir(dst.parent)

    if not can_link:
        if dst.exists() or dst.is_symlink():
            return f"[SKIP] No symlink privilege, leaving existing: {dst}"
        try:
            import shutil
            shutil.copy2(src, dst)
            return f"[COPY] No symlink privilege, copied: {dst}"
        except OSError as exc:
            return f"[ERROR] Copy fallback failed: {dst} ({exc})"

    try:
        if dst.exists() or dst.is_symlink():
            dst.unlink()
        dst.symlink_to(src)
        return f"[LINK] {dst} -> {src}"
    except OSError as exc:
        return f"[ERROR] Failed to create symlink: {dst} -> {src} ({exc})"


# ==========================================================
# VORTEX MOD PACKAGE DISCOVERY (by presence of plugins\MGSHDFix.asi)
# ==========================================================

def iter_two_levels(root: Path) -> Iterable[Path]:
    if not root.exists():
        return
    for level1 in root.iterdir():
        if level1.is_dir():
            for level2 in level1.iterdir():
                if level2.is_dir():
                    yield level2


def find_mod_packages(root: Path) -> List[Path]:
    r"""
    Returns mod package roots (level1) that have a level2 folder containing MGSHDFix.asi,
    which in practice is: <mod>\plugins\MGSHDFix.asi
    """
    out: Dict[str, Path] = {}
    if not root.exists():
        return []

    for level2 in iter_two_levels(root):
        asi = level2 / "MGSHDFix.asi"
        if not asi.exists():
            continue
        mod_root = level2.parent
        out[str(mod_root.resolve()).lower()] = mod_root

    return sorted(out.values(), key=lambda p: p.name.lower())


# ==========================================================
# DEPLOY: LINK MGSHDFix.asi + Config Tool into every mod package
# ==========================================================

def deploy_symlinks_to_all_mods() -> None:
    all_targets: List[Tuple[Path, Path]] = []  # (src, dst)

    for root in ROOTS:
        mods = find_mod_packages(root)
        log(f"[DEPLOY] Mod packages found: {len(mods)} (root: {root})")
        for mod_root in mods:
            plugins_dir = mod_root / "plugins"
            for src, dst_name in DEPLOY_LINKS:
                dst = plugins_dir / dst_name
                all_targets.append((src, dst))

    if not all_targets:
        log("[DEPLOY] No targets found (no matching files at expected depth).")
        return

    max_workers = min(32, (os.cpu_count() or 8) * 2)
    log(f"[DEPLOY] Linking {len(all_targets)} files with {max_workers} workers...")

    with ThreadPoolExecutor(max_workers=max_workers) as ex:
        futures = [ex.submit(ensure_file_symlink, src, dst) for (src, dst) in all_targets]
        for fut in as_completed(futures):
            log(fut.result())


# ==========================================================
# LOG SYMLINKS (MGS2 chosen mod is source; link into all others)
# ==========================================================

def pick_mgs2_source_mod(mgs2_mods: List[Path]) -> Optional[Path]:
    if not mgs2_mods:
        return None

    src_asi_resolved = SRC_ASI.resolve()
    for mod_root in mgs2_mods:
        asi_path = mod_root / "plugins" / "MGSHDFix.asi"
        tgt = resolve_symlink_target(asi_path)
        if tgt is not None and tgt == src_asi_resolved:
            return mod_root

    return mgs2_mods[0]


def link_logs_from_mgs2_source() -> None:
    mgs2_mods = find_mod_packages(ROOT_MGS2)
    log(f"[LOG] MGS2 mod packages found: {len(mgs2_mods)} (root: {ROOT_MGS2})")
    if not mgs2_mods:
        return

    src_mod_root = pick_mgs2_source_mod(mgs2_mods)
    if src_mod_root is None:
        return

    src_logs_dir = src_mod_root / "logs"
    log(f"[LOG] Using MGS2 source mod: {src_mod_root}")
    log(f"[LOG] Source logs dir:      {src_logs_dir}")

    targets = [
        ("MG and MG2", ROOT_MG_MG2),
        ("MGS3", ROOT_MGS3),
    ]

    for label, root in targets:
        mods = find_mod_packages(root)
        log(f"[LOG] {label} mod packages found: {len(mods)} (root: {root})")
        if not mods:
            continue

        for mod_root in mods:
            dst_logs_dir = mod_root / "logs"
            for log_name in LOG_FILES:
                src_log = src_logs_dir / log_name
                dst_log = dst_logs_dir / log_name
                log(ensure_file_symlink(src_log, dst_log))


# ==========================================================
# MAIN
# ==========================================================

def main() -> int:
    log("=== update_local_game_files.py start ===")

    if os.environ.get("CI"):
        log("CI environment detected. Skipping update.")
        return 0

    if not os.environ.get("SHIZ_LOCAL_VORTEX_FILE_SYNC") == "1":
        log("Skipping local Vortex mod file sync.")
        return 0

    log(f"Python: {sys.version}")
    log(f"Script: {Path(__file__).resolve()}")
    log(f"User:   {os.environ.get('USERNAME', '')}")
    log(f"CWD:    {Path.cwd().resolve()}")
    log(f"Args:   {' '.join(sys.argv[1:])}")

    log("Ensuring admin...")
    ensure_admin()
    log("Admin OK.")

    log(f"Checking build outputs:\n  ASI: {SRC_ASI}\n  CFG: {SRC_CFG}")
    if not SRC_ASI.exists() or not SRC_CFG.exists():
        log("ERROR: Source build outputs missing.")
        return 1

    log("Deploying symlinks into Vortex mods...")
    deploy_symlinks_to_all_mods()

    log("Linking MGSHDFix log files (MGS2 is source)...")
    link_logs_from_mgs2_source()

    log("Done.")
    return 0


if __name__ == "__main__":
    rc = 1
    try:
        rc = main()
    except SystemExit as exc:
        code = exc.code if isinstance(exc.code, int) else 0
        log(f"SystemExit: {code}")
        rc = code
        raise
    except Exception as exc:
        log(f"FATAL: {type(exc).__name__}: {exc}")
        raise
    finally:
        log(f"=== exit code {rc} ===")

        if has_flag("--pause"):
            pause()

    raise SystemExit(rc)
