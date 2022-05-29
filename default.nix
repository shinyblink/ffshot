with import <nixpkgs> {};
stdenv.mkDerivation {
  src = ./.;
  name = "ffshot";
  buildInputs = [ xorg.libxcb xorg.xcbutilimage ];
  enableParallelBuilding = true;

  installPhase = ''
      make install PREFIX=$out
    '';
}
