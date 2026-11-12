///////////////////////////////////////////////////////////////////////////////
// Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// The product this build is, chosen when generating with CMake
// (CLASSIC_REBORN_EDITION): "alone", Alone in the Dark: Classic Reborn (the
// trilogy and Jack), or "timegate", Time Gate: Classic Reborn (only Time Gate).
// The same code makes both; only names, icon, cover and the games offered
// change. Plain #defines: the .rc files of the executables read it too.
///////////////////////////////////////////////////////////////////////////////

#pragma once

// CLASSIC_REBORN_REPOSITORY: its GitHub repository (updates, About);
// CLASSIC_REBORN_PACKAGE: the start of its packages' names
#ifdef CLASSIC_REBORN_EDITION_TIMEGATE
#define CLASSIC_REBORN_PRODUCT "Time Gate: Classic Reborn"
#define CLASSIC_REBORN_ICON "../Shared/assets/timegate.ico"
#define CLASSIC_REBORN_REPOSITORY "nayo186/TimeGateClassicReborn"
#define CLASSIC_REBORN_PACKAGE "TimeGateClassicReborn"
#else
#define CLASSIC_REBORN_PRODUCT "Alone in the Dark: Classic Reborn"
#define CLASSIC_REBORN_ICON "../Shared/assets/aitd.ico"
#define CLASSIC_REBORN_REPOSITORY "nayo186/AloneInTheDarkClassicReborn"
#define CLASSIC_REBORN_PACKAGE "ClassicReborn"
#endif
