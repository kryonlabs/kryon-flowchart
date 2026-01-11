{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    gnumake
    pkg-config
    SDL3
    sdl3-ttf
    nim
  ];

  shellHook = ''
    echo "Kryon Plugins Development Environment"
    echo "Available plugins: flowchart, syntax, canvas, storage"
    echo ""
    echo "Build all:"
    echo "  make -C flowchart"
    echo "  make -C syntax"
    echo "  make -C canvas"
    echo "  make -C storage"
  '';
}
