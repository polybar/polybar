#
# spec file for package polybar
# Initially created for openSUSE
#

Name:           polybar
Version:        3.4.1
Release:        0
Summary:        A fast and easy-to-use status bar
License:        MIT
Group:          System/GUI/Other
URL:            https://github.com/polybar/polybar
Source:         https://github.com/polybar/polybar/archive/%{version}.tar.gz
BuildRequires:  clang >= 3.4
BuildRequires:  cmake >= 3.1
BuildRequires:  pkgconfig
BuildRequires:  python-Sphinx
BuildRequires:  python-xml
BuildRequires:  xcb-util-image-devel
BuildRequires:  xcb-util-wm-devel
# optional dependency
BuildRequires:  pkgconfig(alsa)
# main dependency
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(jsoncpp)
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  pkgconfig(libpulse)
BuildRequires:  pkgconfig(python3)
BuildRequires:  pkgconfig(xcb)
BuildRequires:  pkgconfig(xcb-proto)
BuildRequires:  pkgconfig(xcb-util)
%if 0%{?suse_version} <= 1315
BuildRequires:  i3-devel
%else
BuildRequires:  i3-gaps-devel
%endif
%if 0%{?suse_version}
BuildRequires:  libiw-devel
%else
BuildRequires:  wireless-tools-devel
%endif

%description
A fast and easy-to-use status bar for tilling WM

%prep
%setup -q

%build
%cmake
make

%install
%cmake_install

%files
%dir %{_datadir}/bash-completion/
%dir %{_datadir}/bash-completion/completions
%dir %{_datadir}/doc/%{name}
%dir %{_datadir}/zsh/
%dir %{_datadir}/zsh/site-functions
%{_bindir}/%{name}
%{_bindir}/%{name}-msg
%{_datadir}/doc/%{name}/config
%{_mandir}/man1/%{name}.1%{?ext_man}
%{_datadir}/bash-completion/completions/%{name}
%{_datadir}/zsh/site-functions/_%{name}
%{_datadir}/zsh/site-functions/_%{name}_msg

%changelog

