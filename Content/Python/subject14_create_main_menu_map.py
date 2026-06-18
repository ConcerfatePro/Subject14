# Creates /Game/Maps/Lvl_MainMenu (minimal empty level) with Subject14MainMenuGameMode as the
# level's default game mode override, then saves.
#
# Run once from the editor Output Log:
#   py "/home/devin/Documents/Unreal Projects/Subject_14/Content/Python/subject14_create_main_menu_map.py"
#
# After this, set GameDefaultMap to Lvl_MainMenu in DefaultEngine.ini (already done in repo)
# or Project Settings > Maps & Modes.

import unreal

MAP_FOLDER = "/Game/Maps"
MAP_PATH = "/Game/Maps/Lvl_MainMenu"
GM_SOFT = "/Script/Subject_14.Subject14MainMenuGameMode"


def main():
    if not unreal.EditorAssetLibrary.does_directory_exist(MAP_FOLDER):
        unreal.EditorAssetLibrary.make_directory(MAP_FOLDER)

    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        factory = unreal.WorldFactory()
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        world = tools.create_asset("Lvl_MainMenu", "/Game/Maps", unreal.World, factory)
        if not world:
            unreal.log_error("subject14_create_main_menu_map: failed to create Lvl_MainMenu")
            return
        unreal.EditorLoadingAndSavingUtils.save_packages([world.get_outermost()], True)
        unreal.log("subject14_create_main_menu_map: created " + MAP_PATH)
    else:
        unreal.log("subject14_create_main_menu_map: map already exists, updating game mode only")

    unreal.EditorLevelLibrary.load_level(MAP_PATH)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world:
        unreal.log_error("subject14_create_main_menu_map: no editor world after load")
        return

    ws = world.get_world_settings()
    gm_class = unreal.load_class(None, GM_SOFT)
    if gm_class:
        try:
            ws.set_editor_property("DefaultGameMode", gm_class)
        except Exception:
            try:
                ws.set_editor_property("default_game_mode", gm_class)
            except Exception as ex:
                unreal.log_warning(
                    "subject14_create_main_menu_map: could not set DefaultGameMode automatically: "
                    + str(ex)
                    + " — set World Settings > Game Mode Override to Subject14MainMenuGameMode manually."
                )
    else:
        unreal.log_warning(
            "subject14_create_main_menu_map: could not load class "
            + GM_SOFT
            + " — compile C++ first, then re-run this script."
        )

    unreal.EditorLevelLibrary.save_current_level()


if __name__ == "__main__":
    main()
