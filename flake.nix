{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
  };

  outputs = { self, nixpkgs }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
    in {
      devShell.x86_64-linux =
        pkgs.mkShell {
          buildInputs = with pkgs; [
            alsaLib
            cairo.dev
            cmake
            curl.dev
            i3
            jsoncpp.dev
            libmpdclient
            libpulseaudio
            libuv
            pkg-config
            sphinx
            wirelesstools
            xorg.libxcb
            xorg.xcbproto
            xorg.xcbutilimage
            xorg.xcbutilwm.dev
          ];
        };
    };
}
