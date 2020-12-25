Plunge is a command-line utility for Windows and Linux that is used to synchronize sets of files.  (Don't just sync; Plunge!)

Home Page:  https://jeffbourdier.github.io/plunge

### Windows

It is easiest to build Plunge from the Visual Studio solution (`plunge.sln`) included with this repository.  If desired, Plunge can be built from the **Developer Command Prompt** (Run as administrator!) as follows:

	cl plunge.c path.c jb.c /link /OUT:"C:\Program Files (x86)\plunge.exe"

The executable file `plunge.exe` will be output into `C:\Program Files (x86)\`.  (If you want to run Plunge without using the full path, `C:\Program Files (x86)\` can be added to the `PATH` environment variable.)

### Linux

The following command should build Plunge:

	sudo gcc -o /usr/local/bin/plunge plunge.c path.c jb.c

The executable file `plunge` will be output into `/usr/local/bin/`.
