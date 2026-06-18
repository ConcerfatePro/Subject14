# Spawns a native C++ test cube that implements IInteractable (press E while looking at it in PIE).
#
# Prerequisites: compile Subject_14Editor so Subject14InteractableTestActor exists.
# If load_class still fails after a successful compile, fully quit and restart the editor
# (new native UCLASS types are not always visible to Python until the next session).
#
# Run:
#   py "/home/devin/Documents/Unreal Projects/Subject_14/Content/Python/subject14_spawn_test_interactable.py"
#
# Skips if an actor labeled S14_TestInteractable already exists.

import unreal

_ROT_ZERO = unreal.Rotator(0.0, 0.0, 0.0)
_ACTOR_LABEL = "S14_TestInteractable"
_CLASS_PATH = "/Script/Subject_14.Subject14InteractableTestActor"


def _actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def _already_spawned():
    for a in _actor_subsystem().get_all_level_actors():
        try:
            if a.get_actor_label() == _ACTOR_LABEL:
                return True
        except Exception:
            continue
    return False


def _load_actor_class():
    # Ensure the game module is loaded and reflected into Python (fixes many
    # "Failed to find object 'Class /Script/...'" cases when running scripts early).
    try:
        unreal.load_module("Subject_14")
    except Exception as exc:
        unreal.log_warning(
            "Subject14 interact test: load_module(Subject_14) failed (%s); continuing."
            % (exc,)
        )

    cls = unreal.load_class(None, _CLASS_PATH)
    if cls:
        return cls

    # Static find (no disk load) — works if the class is already registered.
    found = unreal.find_object(None, _CLASS_PATH)
    if found:
        return found

    # Generated binding after load_module (often same name as the C++ class).
    binding = getattr(unreal, "Subject14InteractableTestActor", None)
    if binding is not None:
        try:
            return binding.static_class()
        except Exception:
            return binding

    try:
        return unreal.load_object(None, _CLASS_PATH)
    except Exception:
        return None


def main():
    if not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world():
        unreal.log_error("Subject14 interact test: open a level first.")
        return

    if _already_spawned():
        unreal.log("Subject14 interact test: S14_TestInteractable already in level — skip")
        return

    cls = _load_actor_class()
    if not cls:
        unreal.log_error(
            "Subject14 interact test: could not resolve "
            + _CLASS_PATH
            + " — compile Subject_14Editor, then if needed restart the editor."
        )
        return

    # Eye-height aligned with default PlayerStart + first-person camera (~Z 150+).
    # A trace from the camera at Z~160 misses a cube centered at Z=50.
    loc = unreal.Vector(250.0, 0.0, 155.0)
    actor = _actor_subsystem().spawn_actor_from_class(cls, loc, _ROT_ZERO)
    actor.set_actor_label(_ACTOR_LABEL)
    unreal.log(
        "Subject14 interact test: spawned Subject14InteractableTestActor at (250, 0, 155). Save (Ctrl+S)."
    )


main()
