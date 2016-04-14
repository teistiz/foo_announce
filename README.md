# foo_announce
This is a foobar2000 component that sends playback status info via HTTP, in JSON format. It was originally developed for the Instanssi demoparty.

This software is licensed under the terms of the MIT license. See LICENSE for more information.

## Building
First, you'll need the foobar2000 SDK. Get it from the player's home page.

The included Visual Studio solution and project files assume the SDK's been extracted into the same directory this project was cloned into. Depending on your Windows SDK install path and version it may be necessary to modify the project's include directories.

You may find it faster to test your changes if you create a symbolic link from the foobar2000 component directory to the foo_announce.dll in this project's output files.
