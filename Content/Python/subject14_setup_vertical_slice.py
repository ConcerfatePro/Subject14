# Builds the Night 1 -> Day 3 hatch breach vertical slice in the current editor level.
#
# Open Lvl_Dev first, then run:
#   py "/home/devin/Desktop/Unreal Projects/Subject_14/Content/Python/subject14_setup_vertical_slice.py"
#
# Play flow (PIE):
#   1. Night 1 director runs automatically on fresh save (IntroWake -> Night1Active).
#   2. After Night 1 ends, story advances to Day 2 investigation and reloads Lvl_Dev.
#   3. Explore cabin area, trigger Day 2 watched cue, read both notes.
#   4. Walk to hatch advance trigger -> Day 3 breach prep.
#   5. Discover hatch -> breaker -> unlock -> open.

import unreal

_PROJECT_ROOT = "/home/devin/Desktop/Unreal Projects/Subject_14"
_ROT_ZERO = unreal.Rotator(0.0, 0.0, 0.0)

# Story flag names shared with Subject14StorySubsystem / Subject14StoryFlags.
FLAG_DAY2_STARTED = "Day2Started"
FLAG_DAY2_KEY_EVIDENCE = "Day2_KeyEvidence"
FLAG_DAY3_BREACH_PREP = "Day3_BreachPrepStarted"
FLAG_HATCH_HAS_POWER = "HatchHasPower"
FLAG_BREAKER_USED = "BreakerUsed"

_SLICE_LABELS = (
    "S14_DevFloor",
    "S14_PlayerStart",
    "S14_CabinShell",
    "S14_Night1Director",
    "S14_Note01_CanopyMaintenance",
    "S14_Note02_ObservationSummary",
    "S14_Day2_WatchedTrigger",
    "S14_Day3_AdvanceTrigger",
    "S14_DevHatch",
    "S14_DevBreaker",
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


def _setup_floor():
    asub = _actor_subsystem()
    for actor in asub.get_all_level_actors():
        try:
            if actor.get_actor_label() == "S14_DevFloor":
                unreal.log("subject14_setup_vertical_slice: dev floor already present")
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
        floor.set_actor_scale3d(unreal.Vector(50.0, 50.0, 1.0))


def _setup_player_start():
    asub = _actor_subsystem()
    for actor in asub.get_all_level_actors():
        try:
            if actor.get_actor_label() == "S14_PlayerStart":
                unreal.log("subject14_setup_vertical_slice: player start already present")
                return
        except Exception:
            continue
        c = actor.get_class()
        while c:
            if c.get_name() == "PlayerStart":
                unreal.log("subject14_setup_vertical_slice: existing PlayerStart found")
                return
            nxt = c.get_super_class()
            if (not nxt) or (nxt == c):
                break
            c = nxt

    ps = _spawn_actor(unreal.PlayerStart, unreal.Vector(0.0, 0.0, 100.0), unreal.Rotator(0.0, -90.0, 0.0), "S14_PlayerStart")


def _setup_cabin_shell():
    mesh = _load_cube_mesh()
    if not mesh:
        unreal.log_warning("subject14_setup_vertical_slice: could not load Cube mesh for cabin shell")
        return

    cabin = _spawn_actor(unreal.StaticMeshActor, unreal.Vector(0.0, -800.0, 150.0), _ROT_ZERO, "S14_CabinShell")
    smc = cabin.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        smc.set_static_mesh(mesh)
        cabin.set_actor_scale3d(unreal.Vector(5.5, 4.0, 2.8))


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

    director = _spawn_actor(director_cls, unreal.Vector(0.0, -400.0, 50.0), _ROT_ZERO, "S14_Night1Director")
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
    except Exception as exc:
        unreal.log_warning("subject14_setup_vertical_slice: director defaults (%s)" % (exc,))


def _setup_notes():
    note_cls = _load_class(_CLASS_NOTE)
    if not note_cls:
        unreal.log_error("subject14_setup_vertical_slice: could not load NoteActor class")
        return None, None

    note1 = _spawn_actor(note_cls, unreal.Vector(-180.0, -760.0, 120.0), _ROT_ZERO, "S14_Note01_CanopyMaintenance")
    note2 = _spawn_actor(note_cls, unreal.Vector(220.0, -860.0, 120.0), _ROT_ZERO, "S14_Note02_ObservationSummary")

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
            note2.set_editor_property("ObjectiveAfterRead", "Find what is beneath the cabin floor.")
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: note2 defaults (%s)" % (exc,))

    return note1, note2


def _setup_story_triggers():
    trigger_cls = _load_class(_CLASS_TRIGGER)
    if not trigger_cls:
        unreal.log_error("subject14_setup_vertical_slice: could not load StoryTriggerActor class")
        return

    day2 = _spawn_actor(trigger_cls, unreal.Vector(0.0, -650.0, 100.0), _ROT_ZERO, "S14_Day2_WatchedTrigger")
    if day2:
        try:
            day2.set_editor_property("RequiredStoryFlag", unreal.Name(FLAG_DAY2_STARTED))
            day2.set_editor_property("GrantedStoryFlag", unreal.Name("Day2_WatchedCue"))
            day2.set_editor_property("ThoughtLine", "Something out there isn't just wandering. It's looking.")
            day2.set_editor_property("ObjectiveLine", "Search the cabin for anything that feels wrong.")
            day2.set_editor_property("bFireOnOverlap", True)
            day2.set_editor_property("bOneShot", True)
            day2.set_editor_property("bOneShotPersistent", True)
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: day2 trigger (%s)" % (exc,))

    day3 = _spawn_actor(trigger_cls, unreal.Vector(0.0, -780.0, 100.0), _ROT_ZERO, "S14_Day3_AdvanceTrigger")
    if day3:
        try:
            day3.set_editor_property("RequiredStoryFlag", unreal.Name(FLAG_DAY2_KEY_EVIDENCE))
            day3.set_editor_property("GrantedStoryFlag", unreal.Name(FLAG_DAY3_BREACH_PREP))
            day3.set_editor_property("bAdvancePhaseOnFire", True)
            day3.set_editor_property("ThoughtLine", "This was built over something.")
            day3.set_editor_property("ObjectiveLine", "Restore power to the hatch beneath the cabin.")
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

    note = _spawn_actor(note_cls, unreal.Vector(350.0, -720.0, 120.0), _ROT_ZERO, "S14_DevNoteDay3")
    hatch = _spawn_actor(hatch_cls, unreal.Vector(0.0, -800.0, 25.0), _ROT_ZERO, "S14_DevHatch")
    breaker = _spawn_actor(breaker_cls, unreal.Vector(350.0, -650.0, 100.0), _ROT_ZERO, "S14_DevBreaker")

    if note:
        try:
            note.set_editor_property("NoteId", unreal.Name("Note_Day3_Prep"))
            note.set_editor_property("bRegisterInStorySubsystem", True)
            note.set_editor_property("RequiredStoryFlag", unreal.Name(FLAG_DAY3_BREACH_PREP))
            note.set_editor_property("NoteTitle", "Engineering complaint (fragment)")
            note.set_editor_property(
                "NoteBody",
                "If Canopy Ops keeps masking the support resonance, one of these subjects is eventually going to hear it. You cannot build a fake forest on top of steel and expect silence forever.",
            )
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
            breaker.set_editor_property("ObjectiveAfterUse", "Return to the hatch and unlock it.")
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: breaker defaults (%s)" % (exc,))

    if hatch:
        try:
            hatch.set_editor_property("ThoughtOnDiscovery", "This isn't a cabin.")
            hatch.set_editor_property("ThoughtOnNoPower", "It needs power before the lock will release.")
            hatch.set_editor_property("ThoughtOnOpenBlocked", "The seal is still holding.")
        except Exception as exc:
            unreal.log_warning("subject14_setup_vertical_slice: hatch defaults (%s)" % (exc,))

    return hatch, breaker


def _setup_anchors():
    treeline = _spawn_actor(unreal.Actor, unreal.Vector(1200.0, -800.0, 0.0), _ROT_ZERO, "S14_TreelineAnchor")
    creature = _spawn_actor(unreal.Actor, unreal.Vector(900.0, -950.0, 0.0), _ROT_ZERO, "S14_CreatureHintAnchor")
    return treeline, creature


def main():
    if not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world():
        unreal.log_error("subject14_setup_vertical_slice: open Lvl_Dev (or another test map) first.")
        return

    _destroy_labeled(_SLICE_LABELS)

    _setup_floor()
    _setup_player_start()
    _setup_cabin_shell()
    _setup_outdoor_lighting()

    note1, note2 = _setup_notes()
    _setup_story_triggers()
    hatch, breaker = _setup_hatch_slice()
    treeline, creature = _setup_anchors()
    _setup_night1_director(note2 or note1, creature)

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(
        "subject14_setup_vertical_slice: done. Save confirmed. "
        "Fresh test: Subject14.DeleteStory then PIE. "
        "After Night 1: read notes, overlap Day3 trigger, then hatch -> breaker -> open."
    )


main()
