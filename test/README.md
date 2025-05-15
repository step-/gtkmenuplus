# Test environment

_Please note that test scripts are not included in the source distribution archive.
To access the complete test directory, visit_ [GitHub (test)].

## Setup

- Dependencies:

  - Commands: 'defaulttexteditor' in `$PATH`, e.g., symbolic link to leafpad.
  - For interactive tests: packages [yad] >= 14.0 and leafpad.
  - For batch tests: no special dependencies.
  - To develop new batch tests: [jq].

All tests assume a light GTK theme; a dark theme makes the menu less readable.

A few test scripts assume that the test system runs on [Fatdog64] Linux.

## Running test scripts interactively (GUI launcher)

[Yad] 14.0 is needed to run the interactive test launcher.

```sh
    cd this_repository
    [env YAD=path/to/yad14 GTKMENUPLUS=/abs/path/to/gtkmenuplus] \
    test/START &
```

## Running a specific test script interactively

```sh
    [env GTKMENUPLUS=/abs/path/to/gtkmenuplus [GTKMENUPLUS_OPTIONS=...]] \
    test/START test/test_script.gtkmenuplus
```

## Running batch tests

```sh
    test/BATCH
```

Batch tests run without displaying the menu. They compare
a JSON serialization of the menu with a reference file
obtained by invoking gtkmenuplus with options `-j`.

## Test files and resources

Test scripts are located in the `test` directory. JSON serializations
of the test scripts are located in the `test/reference` directory.

To create a new serialization for a test script, run
the script from the GUI launcher and select the "write
reference file" menu entry in the "tools" submenu.

In addition to the test scripts and reference files, The `test` directory
also includes (some hidden) files and directories containing test data.

### Test format

Test files are gtkmenuplus scripts.
To set gtkmenuplus options in the test file, add the following
comment line to the file, and replace the ellipsis with some options.

```sh
    # GTKMENUPLUS_OPTIONS=...
```

Additionally, if the environment variable `GTKMENUPLUS_OPTIONS` is set when the
test runs, its contents are appended to the in-script `GTKMENUPLUS_OPTIONS`.

For some test scripts a corresponding JSON reference file cannot exist,
essentially because these scripts require some form of human assessment.
Column "J" of the GUI launcher tracks the scripts for which a reference
file exists ('`☒`'), should exist ('`☐`') or cannot exist ('&nbsp'). To
mark a test script "cannot", add the following comment line:

```sh
    # SKIP_JSON ...
```

#### Reserved variables

A few gtkmenu variables (`a0`, `mTESTFILE`, `mDIR`, `_instructions_`,
and more for the "tools" menu) are common to all scripts and reserved.

[Fatdog64]: <https://distro.ibiblio.org/fatdog/web>
[GitHub (test)]: <https://github.com/step-/gtkmenuplus/tree/master/test/>
[jq]: <https://github.com/jqlang/jq>
[yad]: <https://github.com/v1cont/yad>
[Yad]: <https://github.com/v1cont/yad>
