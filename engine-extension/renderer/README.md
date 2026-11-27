[object Object]

## Blueprint settings API

The runtime plugin now includes
`Source/BMIGraphicsRuntime/Public/BMIGraphicsSettingsLibrary.h` and its
implementation in `Private/BMIGraphicsSettingsLibrary.cpp`. The static
`UBMIGraphicsSettingsLibrary` functions are exposed to Blueprint under
**Bright Memory | Graphics**, so a UMG settings widget can call the same
runtime commands without embedding console strings in Blueprint graphs.

This is an API seam, not a completed in-game menu: the Bright Memory v1.34
settings-widget assets/source are not present in this repository, and the
library has not been UHT/UBT compiled against the game's exact engine. Do not
claim the controls are visible in the shipped game's menu until the widget is
actually wired and the plugin is compiled into the game.
