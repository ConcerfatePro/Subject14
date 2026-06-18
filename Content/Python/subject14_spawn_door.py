# Spawns Subject14SimpleDoorActor (E toggles open/closed yaw on hinge).
#
# Run:
#   py "/home/devin/Documents/Unreal Projects/Subject_14/Content/Python/subject14_spawn_door.py"

import unreal

_ROT_ZERO = unreal.Rotator(0.0, 0.0, 0.0)
_ACTOR_LABEL = "S14_DevDoor"
_CLASS_PATH = "/Script/Subject_14.Subject14SimpleDoorActor"


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
    try:
        unreal.load_module("Subject_14")
    except Exception as exc:
        unreal.log_warning("Subject14 door: load_module failed (%s)" % (exc,))

    cls = unreal.load_class(None, _CLASS_PATH)
    if cls:
        return cls
    found = unreal.find_object(None, _CLASS_PATH)
    if found:
        return found
    binding = getattr(unreal, "Subject14SimpleDoorActor", None)
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
        unreal.log_error("Subject14 door: open a level first.")
        return
    if _already_spawned():
        unreal.log("Subject14 door: already present — skip")
        return
    cls = _load_actor_class()
    if not cls:
        unreal.log_error("Subject14 door: could not resolve class — compile + restart editor.")
        return
    loc = unreal.Vector(-80.0, 0.0, 0.0)
    actor = _actor_subsystem().spawn_actor_from_class(cls, loc, _ROT_ZERO)
    actor.set_actor_label(_ACTOR_LABEL)
    unreal.log("Subject14 door: spawned S14_DevDoor at (-80,0,0). Save (Ctrl+S).")


main()
