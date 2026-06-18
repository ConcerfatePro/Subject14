# Spawns a minimal Day-3 vertical-slice greybox in the current editor level for PIE testing:
#   - ASubject14NoteActor (labeled S14_DevNoteDay3)
#   - ASubject14CabinHatchActor (labeled S14_DevHatch)
#   - ASubject14BreakerPanelActor (labeled S14_DevBreaker) with LinkedHatches -> hatch
#
# Run after opening Lvl_Dev (or any test map):
#   py "/home/devin/Documents/Unreal Projects/Subject_14/Content/Python/subject14_place_day3_slice.py"
#
# Then in PIE console (tilde):
#   Subject14.SetStoryPhase 4
#   (4 = Day3BreachPrep — hatch unlock macro phase advance requires this.)
#
# Test order: read note -> interact hatch (discover) -> breaker -> hatch (unlock) -> hatch (open).

import unreal

_ROT_ZERO = unreal.Rotator(0.0, 0.0, 0.0)
_LABEL_NOTE = "S14_DevNoteDay3"
_LABEL_HATCH = "S14_DevHatch"
_LABEL_BREAKER = "S14_DevBreaker"

_CLASS_NOTE = "/Script/Subject_14.Subject14NoteActor"
_CLASS_HATCH = "/Script/Subject_14.Subject14CabinHatchActor"
_CLASS_BREAKER = "/Script/Subject_14.Subject14BreakerPanelActor"


def _actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def _load_class(soft_path: str):
    try:
        unreal.load_module("Subject_14")
    except Exception as exc:
        unreal.log_warning("subject14_place_day3_slice: load_module failed (%s)" % (exc,))
    cls = unreal.load_class(None, soft_path)
    if cls:
        return cls
    return unreal.find_object(None, soft_path)


def _spawn(klass, loc, label: str):
    a = _actor_subsystem().spawn_actor_from_class(klass, loc, _ROT_ZERO)
    if a:
        try:
            a.set_actor_label(label)
        except Exception:
            pass
    return a


def main():
    if not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world():
        unreal.log_error("subject14_place_day3_slice: open a level first.")
        return

    note_cls = _load_class(_CLASS_NOTE)
    hatch_cls = _load_class(_CLASS_HATCH)
    breaker_cls = _load_class(_CLASS_BREAKER)
    if not note_cls or not hatch_cls or not breaker_cls:
        unreal.log_error("subject14_place_day3_slice: failed to load one or more classes (compile Subject_14 first).")
        return

    asub = _actor_subsystem()
    for a in asub.get_all_level_actors():
        try:
            lab = a.get_actor_label()
        except Exception:
            continue
        if lab in (_LABEL_NOTE, _LABEL_HATCH, _LABEL_BREAKER):
            try:
                asub.destroy_actor(a)
            except Exception:
                try:
                    unreal.EditorLevelLibrary.destroy_actor(a)
                except Exception:
                    pass

    note = _spawn(note_cls, unreal.Vector(200.0, 0.0, 120.0), _LABEL_NOTE)
    hatch = _spawn(hatch_cls, unreal.Vector(450.0, 0.0, 20.0), _LABEL_HATCH)
    breaker = _spawn(breaker_cls, unreal.Vector(700.0, 180.0, 100.0), _LABEL_BREAKER)
    if not note or not hatch or not breaker:
        unreal.log_error("subject14_place_day3_slice: spawn failed.")
        return

    try:
        note.set_editor_property("NoteId", unreal.Name("Note_Day3_Prep"))
        note.set_editor_property("bRegisterInStorySubsystem", True)
        note.set_editor_property("NoteTitle", "Facility routing (fragment)")
        note.set_editor_property(
            "NoteBody",
            "Auxiliary breaker feeds sub-cabin hatch bus. Simulation prefers you do not look down.",
        )
    except Exception as exc:
        unreal.log_warning("subject14_place_day3_slice: note defaults (%s)" % (exc,))

    try:
        arr = unreal.Array(unreal.Object)
        arr.append(hatch)
        breaker.set_editor_property("LinkedHatches", arr)
    except Exception as exc:
        unreal.log_warning(
            "subject14_place_day3_slice: could not set breaker LinkedHatches (%s). Link manually in editor."
            % (exc,)
        )

    unreal.log(
        "subject14_place_day3_slice: spawned note + hatch + breaker. "
        "Set breaker RequiredPhaseAtLeast to Day3BreachPrep in Details if desired, save level, "
        "then PIE: Subject14.SetStoryPhase 4"
    )


main()
