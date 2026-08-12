Name:           sentinel-desktop
Version:        1.0.0
Release:        1%{?dist}
Summary:        Local-first AI desktop assistant

License:        Apache-2.0
URL:            https://sentinel.dev
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.24
BuildRequires:  gcc-c++
BuildRequires:  ninja-build
BuildRequires:  qt6-qtbase-devel >= 6.5.0
BuildRequires:  qt6-qtdeclarative-devel >= 6.5.0
BuildRequires:  qt6-qtsql-devel >= 6.5.0
BuildRequires:  qt6-qtmultimedia-devel >= 6.5.0
BuildRequires:  qt6-qttools-devel >= 6.5.0
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

Requires:       qt6-qtbase >= 6.5.0
Requires:       qt6-qtdeclarative >= 6.5.0
Requires:       qt6-qtsql >= 6.5.0
Requires:       qt6-qtmultimedia >= 6.5.0
Requires:       hicolor-icon-theme

%description
Sentinel is a cross-platform, local-first AI desktop assistant application
optimized for Fedora KDE Plasma 6. It features local Ollama integration,
memory management, key-value state persistence, and native desktop shell
companionship while ensuring privacy and explicit user control.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DSENTINEL_APP_VERSION="%{version}" \
    -DSENTINEL_BUILD_NUMBER="%{release}"
%cmake_build

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/dev.sentinel.Sentinel.desktop
appstream-util validate-relax --nonet %{buildroot}%{_datadir}/metainfo/dev.sentinel.Sentinel.metainfo.xml
%ctest

%post
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :
%{_bindir}/update-desktop-database &>/dev/null || :

%postun
if [ $1 -eq 0 ] ; then
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :
    %{_bindir}/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
    %{_bindir}/update-desktop-database &>/dev/null || :
fi

%posttrans
%{_bindir}/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :

%files
%license LICENSE
%doc README.md
%{_bindir}/sentinel-desktop
%{_bindir}/sentinel-daemon
%{_bindir}/sentinel-cli
%{_datadir}/applications/dev.sentinel.Sentinel.desktop
%{_datadir}/icons/hicolor/scalable/apps/dev.sentinel.Sentinel.svg
%{_datadir}/icons/hicolor/1024x1024/apps/dev.sentinel.Sentinel.png
%{_datadir}/metainfo/dev.sentinel.Sentinel.metainfo.xml
%{_datadir}/dbus-1/services/dev.sentinel.Sentinel.service
%{_prefix}/lib/systemd/user/sentinel-desktop.service
%config(noreplace) %{_sysconfdir}/sentinel/config.json.template

%changelog
* Thu Aug 06 2026 Sentinel Maintainers <support@sentinel.dev> - 1.0.0-1
- Ship sentinel-daemon and sentinel-cli alongside the desktop app
- systemd user unit now launches the headless sentinel-daemon binary
