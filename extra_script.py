Import("env")
env.Append(LINKFLAGS=["Wl,--no-gc-sections"])