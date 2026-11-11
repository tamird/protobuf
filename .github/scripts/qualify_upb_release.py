"""Run Protobuf's four stage-1 producers on either side of the 37.x backport."""

import hashlib
import json
import os
from pathlib import Path
import subprocess


ROOT = Path(os.environ["RUNNER_TEMP"]) / "upb-37x"
TOOL_ENV = {
    name: os.environ[name]
    for name in ("HOME", "PATH", "USER", "LOGNAME", "LANG", "CC", "CXX", "USE_BAZEL_VERSION")
    if name in os.environ
}
TARGETS = [
    f"//{package}:gen_{message}_upb_proto_{plugin}_stage1"
    for package, message in [("upb/reflection", "descriptor"), ("upb_generator", "plugin")]
    for plugin in ["upb", "upb_minitable"]
]


def output(*args):
    return subprocess.check_output(args, env=TOOL_ENV, text=True).strip()


def digests(paths):
    return {key: hashlib.sha256(path.read_bytes()).hexdigest() for key, path in paths.items()}


def build(arm, phase, *extra):
    diagnostics = ROOT / f"{arm}-{phase}.log"
    command = [
        "bazel", "--batch", "build", "--jobs=4", "--keep_going",
        "--spawn_strategy=processwrapper-sandbox", "--remote_cache=", "--disk_cache=",
        *extra, *TARGETS,
    ]
    if phase == "without-bash":
        # Start the Bazel server in a private mount namespace. The worker's
        # ordinary Bash stays available to Actions and the stock comparison.
        command = [
            "sudo", "unshare", "--mount", "--propagation", "private", "--", "/bin/sh", "-ec",
            'mount --bind "$1" /usr/bin/bash; shift; test ! -x /bin/bash; exec "$@"',
            "mask-bash", str(ROOT / "unavailable-bash"),
            "setpriv", f"--reuid={os.getuid()}", f"--regid={os.getgid()}", "--init-groups",
            "env", "-i", *(f"{k}={v}" for k, v in TOOL_ENV.items()), *command,
        ]
    with diagnostics.open("w") as log:
        code = subprocess.run(command, env=TOOL_ENV, stdout=log, stderr=subprocess.STDOUT).returncode
    if code:
        print("\n".join(diagnostics.read_text().splitlines()[-35:]), flush=True)
    return code


def check(arm, revision, mnemonic):
    subprocess.run(["git", "switch", "--detach", revision], check=True)
    actual = output("git", "rev-parse", "HEAD")
    if actual != revision:
        raise RuntimeError((arm, actual, revision))
    if build(arm, "normal"):
        raise RuntimeError(f"{arm} could not build the four generators with Bash available")
    execroot = Path(output("bazel", "--batch", "info", "execution_root"))
    files = output("bazel", "--batch", "cquery", "set(" + " ".join(TARGETS) + ")", "--output=files")
    paths = {}
    for name in files.splitlines():
        if not name.startswith("bazel-out/") or "/stage1/" not in name:
            raise ValueError(name)
        path = execroot / name
        key = name.split("/bin/", 1)[1]
        paths[key] = path
    if len(paths) < len(TARGETS):
        raise RuntimeError(f"Incomplete output list: {list(paths)}")
    expected = digests(paths)
    # Generate just these outputs again. Their compiled tools and proto inputs
    # stay in Bazel's action cache for the sandboxed runs.
    for path in paths.values():
        path.unlink()
    bep = ROOT / f"{arm}-without-bash.bep"
    code = build(arm, "without-bash",
                 "--build_event_publish_all_actions", f"--build_event_json_file={bep}")
    owners = {}
    for line in bep.read_text().splitlines():
        event = json.loads(line)
        action = event.get("action", {})
        label = event.get("id", {}).get("actionCompleted", {}).get("label", "")
        if "//" in label:
            label = "//" + label.split("//", 1)[1]
        if label in TARGETS and action.get("type") == mnemonic:
            owners[label] = action.get("success", False)
    should_pass = arm == "backport"
    receipt = {"source": actual, "bazelExit": code, "owners": owners, "normalOutputs": expected}
    if should_pass and code == 0:
        receipt["sandboxOutputs"] = digests(paths)
    (ROOT / f"{arm}.json").write_text(json.dumps(receipt, indent=2))
    if owners != dict.fromkeys(TARGETS, should_pass) or (code == 0) != should_pass:
        raise RuntimeError(receipt)
    if should_pass and receipt["sandboxOutputs"] != expected:
        raise RuntimeError("Shell-free generation changed a release output")
    return expected


if __name__ == "__main__":
    ROOT.mkdir()
    (ROOT / "unavailable-bash").touch(mode=0o644)
    if Path("/bin/bash").resolve() != Path("/usr/bin/bash").resolve():
        raise RuntimeError("Blocking /usr/bin/bash would leave /bin/bash available")
    parent = check("release", "0cf7641d26918523b3c26ae8f9b197cbbda05baf", "Genrule")
    backport = check("backport", "3bad9c725b55a39cd6964518c97e959c1924830e", "GenUpbStage1")
    if parent != backport:
        raise RuntimeError("The release and backport generated different bytes")
    print(f"Both generated the same {len(parent)} files. All four backport actions ran without Bash.")
