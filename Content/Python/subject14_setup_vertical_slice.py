# Builds the Night 1 -> Day 3 hatch breach vertical slice in the current editor level.
#
# Open Lvl_Dev first, then run:
#   py "/home/devin/Desktop/Unreal Projects/Subject_14/Content/Python/subject14_setup_vertical_slice.py"
#
# Play flow (PIE):
#   1. Night 1 director runs automatically on fresh save (IntroWake -> Night1Active).
#   2. After Night 1 ends, story advances to Day 2 investigation and reloads Lvl_Dev.
#   3. Walk toward cabin, trigger Day 2 cue, read both notes.
#   4. Enter cabin threshold trigger -> Day 3 breach prep.
#   5. Discover hatch -> breaker -> unlock -> open.

import unreal

_PROJECT_ROOT = "/home/devin/Desktop/Unreal Projects/Subject_14"
_ROT_ZERO = unreal.Rotator(0.0, 0.0, 0.0)
_CABIN_CENTER = unreal.Vector(0.0, -1100.0, 0.0)

FLAG_DAY2_STARTED = "Day2Started"
FLAG_DAY2_KEY_EVIDENCE = "Day2_KeyEvidence"
FLAG_DAY3_BREACH_PREP = "Day3_BreachPrepStarted"
FLAG_HATCH_HAS_POWER = "HatchHasPower"
FLAG_BREAKER_USED = "BreakerUsed"

_SLICE_LABELS = (
    "S14_DevFloor",
    "S14_PlayerStart",
    "S14_CabinShell",
    "S14_CabinDoorFrame",
    "S14_Night1Director",
    "S14_Note01_CanopyMaintenance",
    "S14_Note02_ObservationSummary",
    "S14_Day2_WatchedTrigger",
    "S14_Day3_AdvanceTrigger",
    "S14_DevHatch",
    "S14_DevBreaker",
    "S14_BreakerMarker",
    "S14_DevNoteDay3",
    "S14_Marker_Note01",
    "S14_Marker_Note02",
    "S14_Light_Note01",
    "S14_Light_Note02",
    "S14_Light_Breaker",
    "S14_Light_Hatch",
    "S14_CreatureHintAnchor",
    "S14_TreelineAnchor",
)

_CLASS_NOTE = "/Script/Subject_14.Subject14NoteActor"
_CLASS_HATCH = "/Script/Subject_14.Subject14CabinHatchActor"
_CLASS_BREAKER = "/Script/Subject_14.Subject14BreakerPanelActor"
_CLASS_TRIGGER = "/Script/Subject_14.Subject14StoryTriggerActor"
_CLASS_DIRECTOR = "/Script/Subject_14.Subject14Night1Director"


def _actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def _load_class(soft_path: str):
    try:
        unreal.load_module("Subject_14")
    except Exception as exc:
        unreal.log_warning("subject14_setup_vertical_slice: load_module failed (%s)" % (exc,))
    cls = unreal.load_class(None, soft_path)
    if cls:
        return cls
    return unreal.find_object(None, soft_path)


def _spawn_actor(klass, loc, rot, label: str):
    actor = _actor_subsystem().spawn_actor_from_class(klass, loc, rot)
    if actor:
        try:
            actor.set_actor_label(label)
        except Exception:
            pass
    return actor


def _destroy_labeled(labels):
    asub = _actor_subsystem()
    for actor in asub.get_all_level_actors():
        try:
            lab = actor.get_actor_label()
        except Exception:
            continue
        if lab in labels:
            asub.destroy_actor(actor)


def _is_player_start(actor):
    c = actor.get_class()
    while c:
        try:
            if c.get_name() == "PlayerStart":
                return True
            nxt = c.get_super_class()
        except Exception:
            break
        if (not nxt) or (nxt == c):
            break
        c = nxt
    return False


def _load_static_mesh():
    for soft_path in ("/Engine/BasicShapes/Plane.Plane", "/Engine/BasicShapes/Plane"):
        try:
            mesh = unreal.EditorAssetLibrary.load_asset(soft_path)
            if mesh:
                return mesh
        except Exception:
            pass
    return None


def _load_cube_mesh():
    for soft_path in ("/Engine/BasicShapes/Cube.Cube", "/Engine/BasicShapes/Cube"):
        try:
            mesh = unreal.EditorAssetLibrary.load_asset(soft_path)
            if mesh:
                return mesh
        except Exception:
            pass
    return None


def _spawn_point_light(loc, intensity, label, warm=True, radius=420.0):
    light = _spawn_actor(unreal.PointLight, loc, _ROT_ZERO, label)
    if not light:
        return None
    comp = light.get_component_by_class(unreal.PointLightComponent)
    if comp:
        comp.set_editor_property("intensity", intensity)
        comp.set_editor_property("attenuation_radius", radius)
        comp.set_editor_property("source_radius", 12.0)
        comp.set_editor_property("soft_source_radius", 20.0)
        if warm:
            comp.set_editor_property("light_color", unreal.Color(255, 210, 160, 255))
        else:
            comp.set_editor_property("light_color", unreal.Color(190, 210, 255, 255))
    return light


def _spawn_marker_post(loc, label, tall=1.4):
    mesh = _load_cube_mesh()
    if not mesh:
        return None
    post = _spawn_actor(unreal.StaticMeshActor, loc, _ROT_ZERO, label)
    smc = post.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        smc.set_static_mesh(mesh)
        post.set_actor_scale3d(unreal.Vector(0.18, 0.18, tall))
    return post


def _soften_scene_lights():
    """Tame sun/skylight so point lights read as soft cues, not glare."""
    asub = _actor_subsystem()
    for actor in asub.get_all_level_actors():
        c = actor.get_class()
        while c:
            name = c.get_name()
            if name == "DirectionalLight":
                comp = actor.get_component_by_class(unreal.DirectionalLightComponent)
                if comp:
                    comp.set_editor_property("intensity", 3.2)
                break
            if name == "SkyLight":
                comp = actor.get_component_by_class(unreal.SkyLightComponent)
                if comp:
                    comp.set_editor_property("intensity", 1.1)
                break
            try:
                nxt = c.get_super_class()
            except Exception:
                break
            if (not nxt) or (nxt == c):
                break
            c = nxt


def _setup_floor():
    asub = _actor_subsystem()
    for actor in asub.get_all_level_actors():
        try:
            if actor.get_actor_label() == "S14_DevFloor":
                return
        except Exception:
            continue

    mesh = _load_static_mesh()
    if not mesh:
        unreal.log_error("subject14_setup_vertical_slice: could not load Plane mesh")
        return

    floor = _spawn_actor(unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, 0.0), _ROT_ZERO, "S14_DevFloor")
    smc = floor.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        smc.set_static_mesh(mesh)
        floor.set_actor_scale3d(unreal.Vector(55.0, 55.0, 1.0))


def _ensure_player_start():
    asub = _actor_subsystem()
    for actor in list(asub.get_all_level_actors()):
        try:
            lab = actor.get_actor_label()
        except Exception:
            lab = ""
        if lab == "S14_PlayerStart" or _is_player_start(actor):
            asub.destroy_actor(actor)

    _spawn_actor(
        unreal.PlayerStart,
        unreal.Vector(0.0, 0.0, 100.0),
        unreal.Rotator(0.0, -90.0, 0.0),
        "S14_PlayerStart",
    )


def _setup_cabin_greybox():
    mesh = _load_cube_mesh()
    if not mesh:
        unreal.log_warning("subject14_setup_vertical_slice: could not load Cube mesh for cabin")
        return

    cabin = _spawn_actor(
        unreal.StaticMeshActor,
        _CABIN_CENTER + unreal.Vector(0.0, 0.0, 150.0),
        _ROT_ZERO,
        "S14_CabinShell",
    )
    smc = cabin.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        smc.set_static_mesh(mesh)
        cabin.set_actor_scale3d(unreal.Vector(6.0, 5.0, 3.0))
        try:
            smc.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        except Exception:
            pass

    frame = _spawn_actor(
        unreal.StaticMeshActor,
        _CABIN_CENTER + unreal.Vector(0.0, 250.0, 120.0),
        _ROT_ZERO,
        "S14_CabinDoorFrame",
    )
    fmc = frame.get_component_by_class(unreal.StaticMeshComponent)
    if fmc:
        fmc.set_static_mesh(mesh)
        frame.set_actor_scale3d(unreal.Vector(2.2, 0.35, 2.4))


def _setup_outdoor_lighting():
    import importlib.util

    path = _PROJECT_ROOT + "/Content/Python/subject14_add_outdoor_lighting_rig.py"
    spec = importlib.util.spec_from_file_location("subject14_outdoor_lighting", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.main()


def _setup_night1_director(note_anchor, creature_anchor):
    director_cls = _load_class(_CLASS_DIRECTOR)
    if not director_cls:
        unreal.log_error("subject14_setup_vertical_slice: could not load Night1Director class")
        return

    director = _spawn_actor(director_cls, unreal.Vector(0.0, -350.0, 50.0), _ROT_ZERO, "S14_Night1Director")
    if not director:
        return

    try:
        director.set_editor_property("bAutoStartFromStoryState", True)
        director.set_editor_property("bOnlyRunDuringNight1Phase", True)
        director.set_editor_property("bCommitStoryProgressOnEndNight", True)
        director.set_editor_property("bReturnToMenuAfterNightEnd", True)
        director.set_editor_property("ReturnToMenuMapName", unreal.Name("Lvl_Dev"))
        director.set_editor_property("NoteSuggestedAnchor", note_anchor)
        director.set_editor_property("CreatureHintAnchor", creature_anchor)
        director.set_editor_property("EndNightMessage", "Day breaks.\n\nThe woods are quiet again—for now.")
        director.set_editor_property("EndPromptText", "Press any key to continue")
    except Exception as exc:
        unreal.log_warning("subject14_setup_vertical_slice: director defaults (%s)" % (exc,))


def _setup_notes():
    note_cls = _load_class(_CLASS_NOTE)
    if not note_cls:
        unreal.log_error("subject14_setup_vertical_slice: could not load NoteActor class")
        return None, None

    # Exterior placements (player approaches from +Y). Left utility note, back observation note.
    note1_loc = _CABIN_CENTER + unreal.Vector(-360.0, 180.0, 120.0)
    note2_loc = _CABIN_CENTER + unreal.Vector(0.0, -320.0, 120.0)

    note1 = _spawn_actor(note_cls, note1_loc, _ROT_ZERO, "S14_Note01_CanopyMaintenance")
    note2 = _spawn_actor(note_cls, note2_loc, _ROT_ZERO, "S14_Note02_ObservationSummary")

    if note1:
        try:
            note1.set_editor_property("NoteId", unreal.Name("Note_01_CanopyMaintenance"))
            note1.set_editor_property("bRegisterInStorySubsystem", True)
            note1.set_editor_property("RequiredStoryFlag", unreal.Name(FLAG_DAY2_STARTED))
            note1.set_editor_property("NoteTitle", "Canopy Operations maintenance slip")
            note1.set_editor_property(
                "NoteBody",
                "Sector 14 climate routing remains within tolerance. Minor delay in west-zone rain dispersal. Audio masking loop recalibrated.",
            )
            note1.set_editor_property(
                "ThoughtAfterRead",
                "The rain stopped. For a second, it actually stopped.",
            )
            note1.set_editor_property(
                "ThoughtWhenGateBlocked",
                "Nothing useful here yet.",
            )
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: note1 defaults (%s)" % (exc,))

    if note2:
        try:
            note2.set_editor_property("NoteId", unreal.Name("Note_02_ObservationSummary"))
            note2.set_editor_property("bRegisterInStorySubsystem", True)
            note2.set_editor_property("RequiredStoryFlag", unreal.Name(FLAG_DAY2_STARTED))
            note2.set_editor_property("GrantedStoryFlag", unreal.Name(FLAG_DAY2_KEY_EVIDENCE))
            note2.set_editor_property("NoteTitle", "Observation summary (excerpt)")
            note2.set_editor_property(
                "NoteBody",
                "Subject 14 demonstrates stable environmental adaptation. Cabin remains preferred shelter during initial dark-cycle stress periods.",
            )
            note2.set_editor_property(
                "ThoughtAfterRead",
                "Subject 14...? No. No, that can't be me.",
            )
            note2.set_editor_property("ObjectiveAfterRead", "Something runs under the floor.")
            note2.set_editor_property(
                "ThoughtWhenGateBlocked",
                "Not time to read this yet.",
            )
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: note2 defaults (%s)" % (exc,))

    _spawn_marker_post(note1_loc + unreal.Vector(0.0, 0.0, 70.0), "S14_Marker_Note01", 1.2)
    _spawn_marker_post(note2_loc + unreal.Vector(0.0, 0.0, 70.0), "S14_Marker_Note02", 1.2)
    _spawn_point_light(note1_loc + unreal.Vector(0.0, 0.0, 160.0), 180.0, "S14_Light_Note01", warm=True, radius=380.0)
    _spawn_point_light(note2_loc + unreal.Vector(0.0, 0.0, 160.0), 200.0, "S14_Light_Note02", warm=True, radius=400.0)
    return note1, note2


def _setup_story_triggers():
    trigger_cls = _load_class(_CLASS_TRIGGER)
    if not trigger_cls:
        unreal.log_error("subject14_setup_vertical_slice: could not load StoryTriggerActor class")
        return

    day2 = _spawn_actor(trigger_cls, unreal.Vector(0.0, -520.0, 100.0), _ROT_ZERO, "S14_Day2_WatchedTrigger")
    if day2:
        try:
            day2.set_editor_property("RequiredStoryFlag", unreal.Name(FLAG_DAY2_STARTED))
            day2.set_editor_property("GrantedStoryFlag", unreal.Name("Day2_WatchedCue"))
            day2.set_editor_property("ThoughtLine", "Something out there isn't just wandering. It's looking.")
            day2.set_editor_property("ObjectiveLine", "Read anything left in the cabin.")
            day2.set_editor_property("bFireOnOverlap", True)
            day2.set_editor_property("bOneShot", True)
            day2.set_editor_property("bOneShotPersistent", True)
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: day2 trigger (%s)" % (exc,))

    day3 = _spawn_actor(
        trigger_cls,
        _CABIN_CENTER + unreal.Vector(0.0, 220.0, 90.0),
        _ROT_ZERO,
        "S14_Day3_AdvanceTrigger",
    )
    if day3:
        try:
            day3.set_editor_property("RequiredStoryFlag", unreal.Name(FLAG_DAY2_KEY_EVIDENCE))
            day3.set_editor_property("GrantedStoryFlag", unreal.Name(FLAG_DAY3_BREACH_PREP))
            day3.set_editor_property("bAdvancePhaseOnFire", True)
            day3.set_editor_property("ThoughtLine", "This was built over something.")
            day3.set_editor_property("ObjectiveLine", "Restore local power.")
            day3.set_editor_property("bFireOnOverlap", True)
            day3.set_editor_property("bOneShot", True)
            day3.set_editor_property("bOneShotPersistent", True)
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: day3 trigger (%s)" % (exc,))


def _setup_hatch_slice():
    note_cls = _load_class(_CLASS_NOTE)
    hatch_cls = _load_class(_CLASS_HATCH)
    breaker_cls = _load_class(_CLASS_BREAKER)
    if not note_cls or not hatch_cls or not breaker_cls:
        unreal.log_error("subject14_setup_vertical_slice: hatch slice classes missing")
        return None, None

    hatch_loc = _CABIN_CENTER + unreal.Vector(0.0, 40.0, 35.0)
    breaker_loc = _CABIN_CENTER + unreal.Vector(420.0, 160.0, 110.0)

    note = _spawn_actor(note_cls, breaker_loc + unreal.Vector(80.0, 120.0, 20.0), _ROT_ZERO, "S14_DevNoteDay3")
    hatch = _spawn_actor(hatch_cls, hatch_loc, _ROT_ZERO, "S14_DevHatch")
    breaker = _spawn_actor(breaker_cls, breaker_loc, _ROT_ZERO, "S14_DevBreaker")

    mesh = _load_cube_mesh()
    if mesh:
        marker = _spawn_actor(unreal.StaticMeshActor, breaker_loc + unreal.Vector(0.0, 0.0, 40.0), _ROT_ZERO, "S14_BreakerMarker")
        mmc = marker.get_component_by_class(unreal.StaticMeshComponent)
        if mmc:
            mmc.set_static_mesh(mesh)
            marker.set_actor_scale3d(unreal.Vector(0.45, 0.45, 0.45))

    if note:
        try:
            note.set_editor_property("NoteId", unreal.Name("Note_03_EngineeringComplaint"))
            note.set_editor_property("bRegisterInStorySubsystem", True)
            note.set_editor_property("RequiredStoryFlag", unreal.Name(FLAG_DAY3_BREACH_PREP))
            note.set_editor_property("NoteTitle", "Engineering complaint (fragment)")
            note.set_editor_property(
                "NoteBody",
                "If Canopy Ops keeps masking the support resonance, one of these subjects is eventually going to hear it. You cannot build a fake forest on top of steel and expect silence forever.",
            )
            note.set_editor_property("ObjectiveAfterRead", "The breaker should feed the hatch bus.")
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: day3 note (%s)" % (exc,))

    if breaker and hatch:
        try:
            arr = unreal.Array(unreal.Object)
            arr.append(hatch)
            breaker.set_editor_property("LinkedHatches", arr)
            breaker.set_editor_property("RequiredStoryFlag", unreal.Name(FLAG_DAY3_BREACH_PREP))
            breaker.set_editor_property("GrantedStoryFlag", unreal.Name(FLAG_HATCH_HAS_POWER))
            breaker.set_editor_property("ConsumedStoryFlag", unreal.Name(FLAG_BREAKER_USED))
            breaker.set_editor_property("bSaveImmediatelyAfterUse", True)
            breaker.set_editor_property("ObjectiveAfterUse", "Return to the hatch. Release the lock.")
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: breaker defaults (%s)" % (exc,))

    if hatch:
        try:
            hatch.set_editor_property("RequiredStoryFlag", unreal.Name(FLAG_DAY3_BREACH_PREP))
            hatch.set_editor_property("ThoughtOnDiscovery", "This isn't a cabin.")
            hatch.set_editor_property("ThoughtOnNoPower", "No power. The lock won't release.")
            hatch.set_editor_property("ThoughtWhenGateBlocked", "Not yet. Keep searching.")
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: hatch defaults (%s)" % (exc,))

    _spawn_point_light(breaker_loc + unreal.Vector(0.0, 0.0, 55.0), 220.0, "S14_Light_Breaker", warm=True, radius=350.0)
    _spawn_point_light(hatch_loc + unreal.Vector(0.0, 0.0, 70.0), 120.0, "S14_Light_Hatch", warm=False, radius=280.0)
    return hatch, breaker


def _setup_anchors():
    treeline = _spawn_actor(unreal.Actor, _CABIN_CENTER + unreal.Vector(1400.0, 0.0, 0.0), _ROT_ZERO, "S14_TreelineAnchor")
    creature = _spawn_actor(unreal.Actor, _CABIN_CENTER + unreal.Vector(900.0, -150.0, 0.0), _ROT_ZERO, "S14_CreatureHintAnchor")
    return treeline, creature


def main():
    if not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world():
        unreal.log_error("subject14_setup_vertical_slice: open Lvl_Dev (or another test map) first.")
        return

    _destroy_labeled(_SLICE_LABELS)
    _setup_floor()
    _ensure_player_start()
    _setup_cabin_greybox()
    _setup_outdoor_lighting()
    _soften_scene_lights()

    note1, note2 = _setup_notes()
    _setup_story_triggers()
    hatch, breaker = _setup_hatch_slice()
    treeline, creature = _setup_anchors()
    _setup_night1_director(note2 or note1, creature)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(
        "subject14_setup_vertical_slice: done. Fresh test: Subject14.DeleteStory then PIE in Lvl_Dev."
    )


main()
