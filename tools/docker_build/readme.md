# Description

This docker image contain whole toolset required to compile Gui-Generic for ESP targets using Platform.IO CLI.

By default Gui-Generic is latest version fetched from github, however if you have your own local copy and you want to modify it, it will be better to mount your host's directory containing Gui-Generic and use it instead. In "Run" section there is example how to do it.

There is also /supla/projects directory where you should mount host's directory which contain project you would like to build (see "Run" section).

# Build

```
docker build . -t gui-generic/platformio:local
```

# Run

## Windows

```
docker run --rm -it --mount src=f:/platformio/gui-generic,target=/gui-generic,type=bind gui-generic/platformio:local
```

## Unix

```
docker run --rm -it \ --mount src=/home/user/my_projects,target=/supla/projects,type=bind \ gui-generic/platformio:local
```