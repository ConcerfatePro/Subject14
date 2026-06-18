# Spawns Subject14BatteryPickupActor (E while looking — adds flashlight charge).
#
# Run:
#   py "/home/devin/Documents/Unreal Projects/Subject_14/Content/Python/subject14_spawn_battery_pickup.py"
#
# Skips if an actor labeled S14_BatteryPickup already exists.

import unreal

_ROT_ZERO = unreal.Rotator(0.0, 0.0, 0.0)
_ACTOR_LABEL = "S14_BatteryPickup"
_CLASS_PATH = "/Script/Subject_14.Subject14BatteryPickupActor"


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
        unreal.log_warning(
            "Subject14 battery pickup: load_module(Subject_14) failed (%s); continuing."
            % (exc,)
        )

    cls = unreal.load_class(None, _CLASS_PATH)
    if cls:
        return cls

    found = unreal.find_object(None, _CLASS_PATH)
    if found:
        return found

    binding = getattr(unreal, "Subject14BatteryPickupActor", None)
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
        unreal.log_error("Subject14 battery pickup: open a level first.")
        return

    if _already_spawned():
        unreal.log("Subject14 battery pickup: S14_BatteryPickup already in level — skip")
        return

    cls = _load_actor_class()
    if not cls:
        unreal.log_error(
            "Subject14 battery pickup: could not resolve "
            + _CLASS_PATH
            + " — compile Subject_14Editor, then if needed restart the editor."
        )
        return

    loc = unreal.Vector(200.0, 100.0, 150.0)
    actor = _actor_subsystem().spawn_actor_from_class(cls, loc, _ROT_ZERO)
    actor.set_actor_label(_ACTOR_LABEL)
    unreal.log(
        "Subject14 battery pickup: spawned at (200, 100, 150). Save (Ctrl+S). Drain flashlight, then E on cylinder."
    )


main()
