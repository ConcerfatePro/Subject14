# Next slice after outdoor lighting: greybox play space for PIE.
#
# Run from Output Log:
#   py "/home/devin/Documents/Unreal Projects/Subject_14/Content/Python/subject14_setup_dev_play_space.py"
# Or Tools > Execute Python Script...
#
# - Spawns a large engine Plane mesh (labeled S14_DevFloor) if none with that label exists.
# - Spawns a PlayerStart at origin + Z if the level has zero PlayerStarts.
# Safe to run multiple times.

import unreal

_ROT_ZERO = unreal.Rotator(0.0, 0.0, 0.0)
_FLOOR_LABEL = "S14_DevFloor"
# Try common Engine plane paths (EditorAssetLibrary varies by UE version).
_PLANE_MESH_PATHS = (
    "/Engine/BasicShapes/Plane.Plane",
    "/Engine/BasicShapes/Plane",
)


def _actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def _all_level_actors():
    return _actor_subsystem().get_all_level_actors()


def _native_base_name(actor_class_binding):
    try:
        return actor_class_binding.static_class().get_name()
    except Exception:
        return ""


def _actor_derives_from(actor, engine_short_name: str):
    c = actor.get_class()
    while c:
        try:
            if c.get_name() == engine_short_name:
                return True
            nxt = c.get_super_class()
        except Exception:
            break
        if (not nxt) or (nxt == c):
            break
        c = nxt
    return False


def _count_engine_class(short_name: str):
    return sum(1 for a in _all_level_actors() if _actor_derives_from(a, short_name))


def _spawn(klass, location, rotation):
    return _actor_subsystem().spawn_actor_from_class(klass, location, rotation)


def _has_dev_floor():
    for a in _all_level_actors():
        try:
            if a.get_actor_label() == _FLOOR_LABEL:
                return True
        except Exception:
            continue
    return False


def _load_static_mesh():
    for soft_path in _PLANE_MESH_PATHS:
        try:
            m = unreal.EditorAssetLibrary.load_asset(soft_path)
            if m:
                return m
        except Exception:
            pass
        try:
            m = unreal.load_asset(soft_path)
            if m:
                return m
        except Exception:
            pass
    return None


def _setup_dev_floor():
    if _has_dev_floor():
        unreal.log("Subject14 play space: dev floor already present (label) — skip")
        return
    mesh = _load_static_mesh()
    if not mesh:
        unreal.log_error(
            "Subject14 play space: could not load engine BasicShapes Plane mesh."
        )
        return
    loc = unreal.Vector(0.0, 0.0, 0.0)
    actor = _spawn(unreal.StaticMeshActor, loc, _ROT_ZERO)
    actor.set_actor_label(_FLOOR_LABEL)
    smc = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        unreal.log_error("Subject14 play space: StaticMeshActor has no StaticMeshComponent")
        return
    smc.set_static_mesh(mesh)
    try:
        smc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    except Exception:
        pass
    actor.set_actor_scale3d(unreal.Vector(50.0, 50.0, 1.0))
    unreal.log("Subject14 play space: spawned dev floor (Plane, label S14_DevFloor)")


def _setup_player_start():
    if _count_engine_class("PlayerStart") > 0:
        unreal.log("Subject14 play space: PlayerStart already present — skip")
        return
    # Slightly above the default plane so the capsule clears the surface.
    loc = unreal.Vector(0.0, 0.0, 100.0)
    ps = _spawn(unreal.PlayerStart, loc, _ROT_ZERO)
    ps.set_actor_label("S14_PlayerStart")
    unreal.log("Subject14 play space: spawned PlayerStart at (0, 0, 100)")


def main():
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not w:
        unreal.log_error("Subject14 play space: open a level first.")
        return

    _setup_dev_floor()
    _setup_player_start()
    unreal.log("Subject14 play space: done. Save the level (Ctrl+S).")


main()
