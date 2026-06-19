# Run once from the Unreal Editor Output Log:
#   exec(open(unreal.Paths.project_content_dir() + "Python/subject14_add_outdoor_lighting_rig.py").read())
# Or: Tools > Execute Python Script... and pick this file.
#
# Adds Sky Atmosphere + Directional Light (as sun) + Skylight + Height Fog if missing.
# Safe to run multiple times (skips actors that already exist).

import unreal

# UE Python Rotator has no zero_rotator; use explicit default rotation.
_ROT_ZERO = unreal.Rotator(0.0, 0.0, 0.0)


def _world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def _actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


# UE Python Class wrappers do not expose is_child_of / actor.is_a reliably; walk superclasses by name.
_ENGINE_ACTOR_BASE_NAMES = {
    unreal.SkyAtmosphere: "SkyAtmosphere",
    unreal.DirectionalLight: "DirectionalLight",
    unreal.SkyLight: "SkyLight",
    unreal.ExponentialHeightFog: "ExponentialHeightFog",
}


def _native_engine_class_name(actor_class_binding):
    name = _ENGINE_ACTOR_BASE_NAMES.get(actor_class_binding)
    if name:
        return name
    try:
        return actor_class_binding.static_class().get_name()
    except Exception:
        return ""


def _actor_derives_from_engine_class(actor, actor_class_binding):
    want = _native_engine_class_name(actor_class_binding)
    if not want:
        return False
    c = actor.get_class()
    while c:
        try:
            if c.get_name() == want:
                return True
            nxt = c.get_super_class()
        except Exception:
            break
        if (not nxt) or (nxt == c):
            break
        c = nxt
    return False


def _count(actor_class_binding):
    return sum(
        1
        for a in _actor_subsystem().get_all_level_actors()
        if _actor_derives_from_engine_class(a, actor_class_binding)
    )


def _spawn(klass, location, rotation):
    # UE 5.x: SpawnActorFromClass(ActorClass, Location, Rotation) — editor world comes from the subsystem.
    return _actor_subsystem().spawn_actor_from_class(klass, location, rotation)


def _configure_directional(actor):
    comp = actor.get_component_by_class(unreal.DirectionalLightComponent)
    if not comp:
        unreal.log_warning("Subject14 lighting rig: no DirectionalLightComponent")
        return
    comp.set_editor_property("intensity", 0.85)
    try:
        comp.set_editor_property("light_color", unreal.Color(230, 224, 204, 255))
    except Exception as exc:
        unreal.log_warning("Subject14 lighting rig: could not set directional light_color (%s)" % (exc,))
    # UE 5.x: mark this light as the atmosphere sun (names vary slightly; both are safe if one exists)
    for prop in ("atmosphere_sun_light", "b_atmosphere_sun_light"):
        try:
            comp.set_editor_property(prop, True)
            break
        except Exception:
            continue


def _configure_skylight(actor):
    comp = actor.get_component_by_class(unreal.SkyLightComponent)
    if not comp:
        unreal.log_warning("Subject14 lighting rig: no SkyLightComponent")
        return
    try:
        comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    except Exception:
        pass
    comp.set_editor_property("intensity", 0.28)
    comp.set_editor_property("real_time_capture", True)
    # Fill undersides / contact shadows a little (not pure black under meshes)
    try:
        comp.set_editor_property("lower_hemisphere_is_solid_color", True)
        comp.set_editor_property("lower_hemisphere_solid_color", unreal.LinearColor(0.08, 0.09, 0.12, 1.0))
    except Exception:
        pass


def _configure_height_fog(actor):
    comp = actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if not comp:
        unreal.log_warning("Subject14 lighting rig: no ExponentialHeightFogComponent")
        return
    comp.set_editor_property("fog_density", 0.002)
    # UE 5.7+: FogInscatteringColor was replaced by FogInscatteringLuminance (HDR); Python uses snake_case.
    tint = unreal.LinearColor(0.15, 0.18, 0.25, 1.0)
    for prop in ("fog_inscattering_luminance", "fog_inscattering_color"):
        try:
            comp.set_editor_property(prop, tint)
            break
        except Exception:
            continue


def main():
    w = _world()
    if not w:
        unreal.log_error("Subject14 lighting rig: no editor world (open a level first).")
        return

    loc_origin = unreal.Vector(0.0, 0.0, 0.0)
    rot_sun = unreal.Rotator(-50.0, 35.0, 0.0)

    if _count(unreal.SkyAtmosphere) == 0:
        _spawn(unreal.SkyAtmosphere, loc_origin, _ROT_ZERO)
        unreal.log("Subject14 lighting rig: spawned SkyAtmosphere")
    else:
        unreal.log("Subject14 lighting rig: SkyAtmosphere already present — skip")

    if _count(unreal.DirectionalLight) == 0:
        a = _spawn(unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 200.0), rot_sun)
        _configure_directional(a)
        unreal.log("Subject14 lighting rig: spawned DirectionalLight")
    else:
        for actor in _actor_subsystem().get_all_level_actors():
            if _actor_derives_from_engine_class(actor, unreal.DirectionalLight):
                _configure_directional(actor)
        unreal.log("Subject14 lighting rig: DirectionalLight already present — reconfigured")

    if _count(unreal.SkyLight) == 0:
        a = _spawn(unreal.SkyLight, unreal.Vector(0.0, 0.0, 300.0), _ROT_ZERO)
        _configure_skylight(a)
        unreal.log("Subject14 lighting rig: spawned SkyLight")
    else:
        for actor in _actor_subsystem().get_all_level_actors():
            if _actor_derives_from_engine_class(actor, unreal.SkyLight):
                _configure_skylight(actor)
        unreal.log("Subject14 lighting rig: SkyLight already present — reconfigured")

    if _count(unreal.ExponentialHeightFog) == 0:
        a = _spawn(unreal.ExponentialHeightFog, loc_origin, _ROT_ZERO)
        _configure_height_fog(a)
        unreal.log("Subject14 lighting rig: spawned ExponentialHeightFog")
    else:
        unreal.log("Subject14 lighting rig: ExponentialHeightFog already present — skip")

    unreal.log("Subject14 lighting rig: done. Save the level (Ctrl+S).")


main()
