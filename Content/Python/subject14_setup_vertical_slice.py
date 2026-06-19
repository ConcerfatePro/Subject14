# subject14_setup_vertical_slice.py
#
# Builds a straight-line QA lane for the Night 1 -> Day 2 -> Day 3 -> hatch breach
# vertical slice in Lvl_Dev.
#
# Run in Output Log (Python mode — dropdown on the left must say Python, not Cmd):
#   import unreal
#   exec(open(unreal.Paths.project_content_dir() + "Python/subject14_setup_vertical_slice.py").read())
#
# Safe to re-run: destroys all QA_ and S14_ managed actors first.
#
# QA LANE ORDER (player walks south / -Y from spawn):
#   [Spawn] → [1]Note1 → [2]Note2 → [3]WatchedTrigger → [4]Day3PrepTrigger
#           → [5]Hatch(NoPower) → [6]Note3 → [7]Breaker → (walk back) → [5]Hatch(Unlock+Open)

import os
import importlib.util

import unreal

# ---------------------------------------------------------------------------
# Story flags (must match Subject14StorySubsystem.h namespace)
# ---------------------------------------------------------------------------
FLAG_DAY2_STARTED      = "Day2Started"
FLAG_DAY2_KEY_EVIDENCE = "Day2_KeyEvidence"
FLAG_DAY3_BREACH_PREP  = "Day3_BreachPrepStarted"
FLAG_HATCH_HAS_POWER   = "HatchHasPower"
FLAG_BREAKER_USED      = "BreakerUsed"

# ---------------------------------------------------------------------------
# QA lane layout
#
# Player spawns at Y=+300 facing -Y (yaw = -90).
# Each station is further in the -Y direction.
# Spacing: ~450–500 UU between interactive stations.
# Marker posts are offset +130 in X so they never occlude the interact trace.
# ---------------------------------------------------------------------------
_LANE_X   =    0.0   # straight line along X=0
_LANE_Z   =  100.0   # standing/eye-height for notes, triggers, breaker
_HATCH_Z  =    5.0   # floor-level for hatch lid

_Y_SPAWN  =   300.0  # player start
_Y_NOTE1  =  -200.0  # Station 1
_Y_NOTE2  =  -700.0  # Station 2
_Y_WATCH  = -1150.0  # Station 3  (overlap trigger)
_Y_DAY3   = -1550.0  # Station 4  (overlap trigger, advances phase)
_Y_HATCH  = -1950.0  # Station 5 + 9  (same hatch, visited twice)
_Y_NOTE3  = -2350.0  # Station 6
_Y_BREAK  = -2750.0  # Station 7

_MRK_DX   =  130.0   # marker post X offset (right-hand side as player walks -Y)

# Cabin greybox off to the side — decorative only, no collision
_CABIN_LOC = unreal.Vector(2200.0, -1200.0, 150.0)

_ROT_ZERO = unreal.Rotator(0.0, 0.0, 0.0)

# ---------------------------------------------------------------------------
# Actor class soft-object paths
# ---------------------------------------------------------------------------
_CLASS_NOTE    = "/Script/Subject_14.Subject14NoteActor"
_CLASS_HATCH   = "/Script/Subject_14.Subject14CabinHatchActor"
_CLASS_BREAKER = "/Script/Subject_14.Subject14BreakerPanelActor"
_CLASS_TRIGGER = "/Script/Subject_14.Subject14StoryTriggerActor"
_CLASS_DIRECTOR= "/Script/Subject_14.Subject14Night1Director"

# ---------------------------------------------------------------------------
# All actor labels managed by this script.
# _destroy_managed() destroys any actor whose label is in this set
# OR whose label starts with "QA_" or "S14_Light_".
# ---------------------------------------------------------------------------
_MANAGED_LABELS = frozenset((
    # infrastructure
    "S14_DevFloor",
    "S14_PlayerStart",
    "S14_CabinShell",
    "S14_CabinDoorFrame",
    "S14_Night1Director",
    "S14_PostProcess",
    "S14_CreatureHintAnchor",
    "S14_TreelineAnchor",
    # old scattered layout (kept so reruns after old scripts are clean)
    "S14_Note01_CanopyMaintenance",
    "S14_Note02_ObservationSummary",
    "S14_Day2_WatchedTrigger",
    "S14_Day3_AdvanceTrigger",
    "S14_DevHatch",
    "S14_DevBreaker",
    "S14_DevNoteDay3",
    "S14_HatchPad",
    "S14_Marker_Note01",
    "S14_Marker_Note02",
    "S14_Marker_Hatch",
    "S14_Marker_Breaker",
    "S14_BreakerMarker",
))

# ===========================================================================
# Internal helpers
# ===========================================================================

def _asub():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def _load_class(path):
    try:
        unreal.load_module("Subject_14")
    except Exception:
        pass
    cls = unreal.load_class(None, path)
    if cls:
        return cls
    return unreal.find_object(None, path)


def _spawn(klass, loc, rot, label):
    actor = _asub().spawn_actor_from_class(klass, loc, rot)
    if actor:
        try:
            actor.set_actor_label(label)
        except Exception:
            pass
    return actor


def _prop(actor, name, value):
    """Set an editor property; log a warning on failure (never raises)."""
    try:
        actor.set_editor_property(name, value)
    except Exception as exc:
        lbl = ""
        try:
            lbl = actor.get_actor_label()
        except Exception:
            pass
        unreal.log_warning(
            "subject14_setup_vertical_slice: set_prop %s=%r on '%s': %s" % (name, value, lbl, exc)
        )


def _is_player_start(actor):
    c = actor.get_class()
    while c:
        try:
            if c.get_name() == "PlayerStart":
                return True
            nxt = c.get_super_class()
        except Exception:
            break
        if not nxt or nxt == c:
            break
        c = nxt
    return False


def _cube():
    for p in ("/Engine/BasicShapes/Cube.Cube", "/Engine/BasicShapes/Cube"):
        try:
            m = unreal.EditorAssetLibrary.load_asset(p)
            if m:
                return m
        except Exception:
            pass
    return None


def _plane():
    for p in ("/Engine/BasicShapes/Plane.Plane", "/Engine/BasicShapes/Plane"):
        try:
            m = unreal.EditorAssetLibrary.load_asset(p)
            if m:
                return m
        except Exception:
            pass
    return None


def _no_col(smc):
    if smc:
        try:
            smc.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        except Exception:
            pass


# ===========================================================================
# Scene management
# ===========================================================================

def _destroy_managed():
    """Remove all actors from previous runs so the script is idempotent."""
    asub = _asub()
    for actor in list(asub.get_all_level_actors()):
        try:
            lab = actor.get_actor_label()
        except Exception:
            continue
        if lab in _MANAGED_LABELS or lab.startswith("QA_") or lab.startswith("S14_Light_"):
            asub.destroy_actor(actor)


def _setup_floor():
    mesh = _plane()
    if not mesh:
        unreal.log_error("subject14_setup_vertical_slice: could not load Plane mesh for floor")
        return
    # Centre the floor under the full QA lane
    center_y = (_Y_SPAWN + _Y_BREAK) * 0.5   # ~-1225
    floor = _spawn(unreal.StaticMeshActor, unreal.Vector(0.0, center_y, 0.0), _ROT_ZERO, "S14_DevFloor")
    smc = floor.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        smc.set_static_mesh(mesh)
        floor.set_actor_scale3d(unreal.Vector(80.0, 80.0, 1.0))


def _ensure_player_start():
    asub = _asub()
    for actor in list(asub.get_all_level_actors()):
        try:
            lab = actor.get_actor_label()
        except Exception:
            lab = ""
        if lab == "S14_PlayerStart" or _is_player_start(actor):
            asub.destroy_actor(actor)
    _spawn(
        unreal.PlayerStart,
        unreal.Vector(_LANE_X, _Y_SPAWN, 100.0),
        unreal.Rotator(0.0, -90.0, 0.0),   # yaw -90 = face -Y = face the lane
        "S14_PlayerStart",
    )


def _setup_cabin_greybox():
    """Decorative cabin off to the side of the QA lane — no collision."""
    mesh = _cube()
    if not mesh:
        return
    cabin = _spawn(unreal.StaticMeshActor, _CABIN_LOC, _ROT_ZERO, "S14_CabinShell")
    smc = cabin.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        smc.set_static_mesh(mesh)
        cabin.set_actor_scale3d(unreal.Vector(6.0, 5.0, 3.0))
        _no_col(smc)
    frame = _spawn(
        unreal.StaticMeshActor,
        _CABIN_LOC + unreal.Vector(0.0, 250.0, -30.0),
        _ROT_ZERO,
        "S14_CabinDoorFrame",
    )
    fmc = frame.get_component_by_class(unreal.StaticMeshComponent)
    if fmc:
        fmc.set_static_mesh(mesh)
        frame.set_actor_scale3d(unreal.Vector(2.2, 0.35, 2.4))
        _no_col(fmc)


# ===========================================================================
# Lighting
# ===========================================================================

def _setup_outdoor_lighting():
    path = os.path.join(
        unreal.Paths.project_content_dir(), "Python", "subject14_add_outdoor_lighting_rig.py"
    )
    spec = importlib.util.spec_from_file_location("s14_outdoor_lighting", path)
    mod  = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    mod.main()


def _soften_scene_lights():
    """Dim directional/skylight; remove any stray point lights created by old runs."""
    asub = _asub()
    for actor in list(asub.get_all_level_actors()):
        try:
            lab = actor.get_actor_label()
        except Exception:
            lab = ""
        if lab.startswith("S14_Light_"):
            asub.destroy_actor(actor)
            continue
        dlc = actor.get_component_by_class(unreal.DirectionalLightComponent)
        if dlc:
            dlc.set_editor_property("intensity", 0.85)
            try:
                dlc.set_editor_property("light_color", unreal.Color(230, 224, 204, 255))
            except Exception:
                pass
        slc = actor.get_component_by_class(unreal.SkyLightComponent)
        if slc:
            slc.set_editor_property("intensity", 0.28)
            try:
                slc.set_editor_property("real_time_capture", False)
            except Exception:
                pass


def _setup_post_process():
    """Infinite post-process volume: no bloom, no lens flare, mild exposure clamp."""
    def _try(target, names, value):
        for n in names:
            try:
                target.set_editor_property(n, value)
                return True
            except Exception:
                pass
        return False

    pp = _spawn(unreal.PostProcessVolume, unreal.Vector(0.0, 0.0, 200.0), _ROT_ZERO, "S14_PostProcess")
    if not pp:
        return

    # Mark as infinite / unbound
    for prop in ("b_unbound", "unbound", "b_infinite_extent"):
        try:
            pp.set_editor_property(prop, True)
            break
        except Exception:
            pass
    for prop, val in (("blend_weight", 1.0), ("blend_radius", 0.0)):
        try:
            pp.set_editor_property(prop, val)
        except Exception:
            pass

    # Get the PostProcessSettings struct
    settings = None
    try:
        comp = pp.get_component_by_class(unreal.PostProcessComponent)
        if comp:
            settings = comp.get_editor_property("settings")
    except Exception:
        pass
    if settings is None:
        try:
            settings = pp.get_editor_property("settings")
        except Exception:
            pass
    if settings is None:
        unreal.log_warning("subject14_setup_vertical_slice: PostProcessSettings unavailable — skipping PP config")
        return

    _try(settings, ("override_bloom_intensity",        "b_override_bloom_intensity"),        True)
    _try(settings, ("bloom_intensity",),                                                      0.0)
    _try(settings, ("override_lens_flare_intensity",   "b_override_lens_flare_intensity"),   True)
    _try(settings, ("lens_flare_intensity",),                                                 0.0)
    _try(settings, ("override_auto_exposure_bias",     "b_override_auto_exposure_bias"),     True)
    _try(settings, ("auto_exposure_bias",),                                                  -0.35)

    try:
        pp.set_editor_property("settings", settings)
    except Exception:
        pass


# ===========================================================================
# Marker helpers
# ===========================================================================

def _marker_post(loc, label, scale=(0.18, 0.18, 2.4)):
    """Tall thin no-collision post beside a QA station."""
    mesh = _cube()
    if not mesh:
        return None
    actor = _spawn(unreal.StaticMeshActor, loc, _ROT_ZERO, label)
    if not actor:
        return None
    smc = actor.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        smc.set_static_mesh(mesh)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        _no_col(smc)
    return actor


def _station_locs(y, z=None):
    """Return (actor_loc, marker_loc) for a QA station at lane position y."""
    az = z if z is not None else _LANE_Z
    return (
        unreal.Vector(_LANE_X, y, az),
        unreal.Vector(_LANE_X + _MRK_DX, y, _LANE_Z),
    )


# ===========================================================================
# Night 1 Director
# ===========================================================================

def _setup_night1_director():
    cls = _load_class(_CLASS_DIRECTOR)
    if not cls:
        unreal.log_error("subject14_setup_vertical_slice: Night1Director class not found — rebuild C++")
        return None

    # Place director behind the player spawn (it runs the Night1 sequence; not in the lane)
    director = _spawn(cls, unreal.Vector(0.0, 600.0, 50.0), _ROT_ZERO, "S14_Night1Director")
    if not director:
        return None

    _prop(director, "bAutoStartFromStoryState",      True)
    _prop(director, "bOnlyRunDuringNight1Phase",      True)
    _prop(director, "bCommitStoryProgressOnEndNight", True)
    _prop(director, "bReturnToMenuAfterNightEnd",     True)
    _prop(director, "ReturnToMenuMapName",            unreal.Name("Lvl_Dev"))
    return director


# ===========================================================================
# QA lane stations
# ===========================================================================

def _qa_note(actor_label, marker_label, y,
             note_id, title, body,
             required_flag,
             granted_flag=None,
             thought_after=None,
             objective_after=None,
             gate_thought=None):
    """Spawn one NoteActor station with a side marker post."""
    note_cls = _load_class(_CLASS_NOTE)
    if not note_cls:
        unreal.log_error("subject14_setup_vertical_slice: NoteActor class not found")
        return None

    actor_loc, mrk_loc = _station_locs(y)
    note = _spawn(note_cls, actor_loc, _ROT_ZERO, actor_label)
    if note:
        _prop(note, "NoteId",                  unreal.Name(note_id))
        _prop(note, "bRegisterInStorySubsystem", True)
        _prop(note, "RequiredStoryFlag",        unreal.Name(required_flag))
        _prop(note, "NoteTitle",                title)
        _prop(note, "NoteBody",                 body)
        if granted_flag:
            _prop(note, "GrantedStoryFlag",     unreal.Name(granted_flag))
        if thought_after:
            _prop(note, "ThoughtAfterRead",     thought_after)
        if objective_after:
            _prop(note, "ObjectiveAfterRead",   objective_after)
        if gate_thought:
            _prop(note, "ThoughtWhenGateBlocked", gate_thought)

    _marker_post(mrk_loc, marker_label)
    return note


def _qa_trigger(actor_label, marker_label, y,
                required_flag, granted_flag,
                thought, objective,
                advance_phase=False):
    """Spawn one StoryTriggerActor station (overlap-only, does NOT block interact traces)."""
    cls = _load_class(_CLASS_TRIGGER)
    if not cls:
        unreal.log_error("subject14_setup_vertical_slice: StoryTriggerActor class not found")
        return None

    actor_loc, mrk_loc = _station_locs(y)
    trigger = _spawn(cls, actor_loc, _ROT_ZERO, actor_label)
    if trigger:
        _prop(trigger, "RequiredStoryFlag",   unreal.Name(required_flag))
        _prop(trigger, "GrantedStoryFlag",    unreal.Name(granted_flag))
        _prop(trigger, "ThoughtLine",         thought)
        _prop(trigger, "ObjectiveLine",       objective)
        _prop(trigger, "bFireOnOverlap",      True)
        _prop(trigger, "bOneShot",            True)
        _prop(trigger, "bOneShotPersistent",  True)
        if advance_phase:
            _prop(trigger, "bAdvancePhaseOnFire", True)

    _marker_post(mrk_loc, marker_label)
    return trigger


def _setup_qa_lane():
    """Build all 7+1 QA stations in order along the -Y lane."""
    hatch_cls   = _load_class(_CLASS_HATCH)
    breaker_cls = _load_class(_CLASS_BREAKER)
    if not hatch_cls or not breaker_cls:
        unreal.log_error(
            "subject14_setup_vertical_slice: Hatch or Breaker class not found — rebuild C++ first"
        )
        return None

    # ------------------------------------------------------------------
    # Station 1 — Note 1: Canopy Ops maintenance slip
    # Required: Day2Started (set by Night1Director on night end)
    # ------------------------------------------------------------------
    _qa_note(
        actor_label   = "QA_01_Note_CanopyOps",
        marker_label  = "QA_01_Marker",
        y             = _Y_NOTE1,
        note_id       = "Note_01_CanopyMaintenance",
        title         = "[QA-1] Canopy Operations maintenance slip",
        body          = (
            "Sector 14 climate routing remains within tolerance. "
            "Minor delay in west-zone rain dispersal. "
            "Audio masking loop recalibrated."
        ),
        required_flag = FLAG_DAY2_STARTED,
        thought_after = "The rain stopped. For a second, it actually stopped.",
        gate_thought  = "Nothing useful here yet.",
    )

    # ------------------------------------------------------------------
    # Station 2 — Note 2: Subject 14 observation  (grants key evidence)
    # ------------------------------------------------------------------
    _qa_note(
        actor_label    = "QA_02_Note_Subject14",
        marker_label   = "QA_02_Marker",
        y              = _Y_NOTE2,
        note_id        = "Note_02_ObservationSummary",
        title          = "[QA-2] Observation summary (excerpt)",
        body           = (
            "Subject 14 demonstrates stable environmental adaptation. "
            "Cabin remains preferred shelter during initial dark-cycle stress periods."
        ),
        required_flag  = FLAG_DAY2_STARTED,
        granted_flag   = FLAG_DAY2_KEY_EVIDENCE,
        thought_after  = "Subject 14...? No. No, that can't be me.",
        objective_after= "Follow the wiring.",
        gate_thought   = "Not time to read this yet.",
    )

    # ------------------------------------------------------------------
    # Station 3 — Watched trigger (overlap, fires Day2_WatchedCue)
    # ------------------------------------------------------------------
    _qa_trigger(
        actor_label  = "QA_03_WatchedTrigger",
        marker_label = "QA_03_Marker",
        y            = _Y_WATCH,
        required_flag= FLAG_DAY2_STARTED,
        granted_flag = "Day2_WatchedCue",
        thought      = "Something out there isn't just wandering. It's looking.",
        objective    = "Search the cabin area.",
    )

    # ------------------------------------------------------------------
    # Station 4 — Day 3 prep trigger (overlap, grants Day3_BreachPrepStarted
    #             and advances story phase)
    # Requires Day2_KeyEvidence so player must read Note 2 first.
    # ------------------------------------------------------------------
    _qa_trigger(
        actor_label   = "QA_04_Day3PrepTrigger",
        marker_label  = "QA_04_Marker",
        y             = _Y_DAY3,
        required_flag = FLAG_DAY2_KEY_EVIDENCE,
        granted_flag  = FLAG_DAY3_BREACH_PREP,
        thought       = "This was built over something.",
        objective     = "Restore local power.",
        advance_phase = True,
    )

    # ------------------------------------------------------------------
    # Station 5 (and 9) — Hatch  (ONE actor, visited twice)
    #   First visit:  no power → "No power. The lock won't release."
    #   After breaker: E again → unlock + open → FirstBreach
    #
    # The InteractProxy sphere (r=180) gives a generous aim target.
    # The DiscoveryVolume (r=140) fires the proximity cue when near.
    # ------------------------------------------------------------------
    hatch_loc = unreal.Vector(_LANE_X, _Y_HATCH, _HATCH_Z)
    hatch = _spawn(hatch_cls, hatch_loc, _ROT_ZERO, "QA_05_Hatch")
    if hatch:
        _prop(hatch, "RequiredStoryFlag",       unreal.Name(FLAG_DAY3_BREACH_PREP))
        _prop(hatch, "ThoughtOnDiscovery",      "This isn't a cabin.")
        _prop(hatch, "ThoughtOnNoPower",        "No power. The lock won't release.")
        _prop(hatch, "ThoughtOnUnlock",         "Lock released.")
        _prop(hatch, "ThoughtWhenGateBlocked",  "Not yet. Keep searching.")

    # Right-side marker at Station 5
    _marker_post(
        unreal.Vector(_LANE_X + _MRK_DX, _Y_HATCH, _LANE_Z),
        "QA_05_Marker",
    )
    # Station 8 — "Return to hatch" visual marker on the LEFT side of the hatch.
    # When the player turns around after the breaker, this tall post is visible
    # across the lane pointing them back to the hatch.
    _marker_post(
        unreal.Vector(_LANE_X - _MRK_DX * 2.0, _Y_HATCH, _LANE_Z),
        "QA_08_ReturnToHatchMarker",
        scale=(0.40, 0.40, 3.5),   # extra tall so it's visible from the breaker end
    )

    # ------------------------------------------------------------------
    # Station 6 — Note 3: engineering / breaker clue
    # Required: Day3_BreachPrepStarted
    # ------------------------------------------------------------------
    _qa_note(
        actor_label    = "QA_06_Note_BreakerClue",
        marker_label   = "QA_06_Marker",
        y              = _Y_NOTE3,
        note_id        = "Note_03_EngineeringComplaint",
        title          = "[QA-6] Engineering complaint (fragment)",
        body           = (
            "If Canopy Ops keeps masking the support resonance, one of these subjects "
            "is eventually going to hear it. "
            "You cannot build a fake forest on top of steel and expect silence forever."
        ),
        required_flag  = FLAG_DAY3_BREACH_PREP,
        objective_after= "Use the breaker ahead.",
    )

    # ------------------------------------------------------------------
    # Station 7 — Breaker panel
    # Required: Day3_BreachPrepStarted
    # Grants: HatchHasPower + BreakerUsed  → objective "Return to the hatch."
    # ------------------------------------------------------------------
    breaker_loc = unreal.Vector(_LANE_X, _Y_BREAK, _LANE_Z)
    breaker = _spawn(breaker_cls, breaker_loc, _ROT_ZERO, "QA_07_BreakerPanel")
    if breaker:
        if hatch:
            try:
                arr = unreal.Array(unreal.Object)
                arr.append(hatch)
                breaker.set_editor_property("LinkedHatches", arr)
            except Exception as exc:
                unreal.log_warning(
                    "subject14_setup_vertical_slice: breaker LinkedHatches (%s)" % (exc,)
                )
        _prop(breaker, "RequiredStoryFlag",      unreal.Name(FLAG_DAY3_BREACH_PREP))
        _prop(breaker, "GrantedStoryFlag",       unreal.Name(FLAG_HATCH_HAS_POWER))
        _prop(breaker, "ConsumedStoryFlag",      unreal.Name(FLAG_BREAKER_USED))
        _prop(breaker, "bSaveImmediatelyAfterUse", True)
        _prop(breaker, "ObjectiveAfterUse",      "Return to the hatch.")

    _marker_post(
        unreal.Vector(_LANE_X + _MRK_DX, _Y_BREAK, _LANE_Z),
        "QA_07_Marker",
    )

    unreal.log(
        "subject14_setup_vertical_slice: QA lane spawned — "
        "7 stations from Y=%.0f to Y=%.0f (player walks -Y)." % (_Y_NOTE1, _Y_BREAK)
    )
    return hatch


# ===========================================================================
# Entry point
# ===========================================================================

def main():
    if not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world():
        unreal.log_error("subject14_setup_vertical_slice: no editor world — open Lvl_Dev first.")
        return

    _destroy_managed()

    _setup_floor()
    _ensure_player_start()
    _setup_cabin_greybox()

    _setup_outdoor_lighting()
    _soften_scene_lights()
    _setup_post_process()

    _setup_qa_lane()
    _setup_night1_director()

    # Second soften pass — outdoor lighting rig may have reset values
    _soften_scene_lights()

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    unreal.log(
        "\n"
        "====================================================\n"
        "  subject14_setup_vertical_slice: DONE\n"
        "====================================================\n"
        "  QA lane: walk FORWARD (-Y) from spawn.\n"
        "  [1] QA_01_Note_CanopyOps      Y=%.0f\n"
        "  [2] QA_02_Note_Subject14      Y=%.0f\n"
        "  [3] QA_03_WatchedTrigger      Y=%.0f  (walk through)\n"
        "  [4] QA_04_Day3PrepTrigger     Y=%.0f  (walk through)\n"
        "  [5] QA_05_Hatch               Y=%.0f  (E → no power)\n"
        "  [6] QA_06_Note_BreakerClue    Y=%.0f\n"
        "  [7] QA_07_BreakerPanel        Y=%.0f  (E → power on)\n"
        "      Walk BACK to Y=%.0f\n"
        "  [9] QA_05_Hatch (same)        Y=%.0f  (E → unlock → open → FirstBreach)\n"
        "====================================================\n"
        "  Console: Subject14.DeleteStory  then PIE.\n"
        "  Debug:   Subject14.DumpStory\n"
        "====================================================" % (
            _Y_NOTE1, _Y_NOTE2, _Y_WATCH, _Y_DAY3,
            _Y_HATCH, _Y_NOTE3, _Y_BREAK,
            _Y_HATCH, _Y_HATCH,
        )
    )


main()
