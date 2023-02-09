-- premake5.lua
workspace "CashewApp"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "CashewApp"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
include "Walnut/WalnutExternal.lua"

include "CashewApp"