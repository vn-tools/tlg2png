Converts TLG images to PNG.

TLG images are used by games based on Kirikiri engine, which includes titles
such as Fate/Stay Night.

There are two implementations: C++ and Ruby. C++ is *much* faster, but since
nontechnical people might have hard time compiling it, I've kept Ruby version
around.

C++ version requires libpng. Ruby version depends on RMagick gem.
