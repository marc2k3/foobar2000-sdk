# foobar2000-sdk

You can always get an original untouched copy here: https://foobar2000.org/SDK

This copy has been updated for use with `Visual Studio 2022 Preview 2` and later. This means components built with it should not be released for others to use.

Changes from the original:

- All projects updated for `Visual Studio 2022 Preview 2` and above  by using the `v143` platform toolset and latest Windows 10 SDK.
- By default it targets [foobar2000](https://foobar2000.org) `v1.5` and above. Edit `foobar2000\SDK\foobar2000.h` if you wish to change this.
- All projects have these additional compiler options set: `/permissive- /std:c++latest`.
- Additonally, `pfc` has these adddtional compiler options: `/Zc:char8_t- /utf-8`.
- `libPPUI`, `SDK helpers` and `foo_sample` are pre-configured to include the bundled copy of [WTL](https://sourceforge.net/projects/wtl/) so everything should just work.

Licenses for each project can be found in their respective folders.
