with import <nixpkgs> {};
stdenv.mkDerivation {
  name = "ffshot";
  buildInputs = [ xorg.libxcb xorg.xcbutilimage ];
}
