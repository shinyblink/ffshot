with import <nixpkgs> {};
pkgs.mkShell {
  buildInputs = [ xorg.libxcb xorg.xcbutilimage ];
}
