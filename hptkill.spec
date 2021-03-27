%define reldate 20210327
%define reltype C
# may be one of: C (current), R (release), S (stable)

Name: hptkill
Version: 1.9.%{reldate}%{reltype}
Release: 1
Group: Applications/FTN
Summary: hptkill - Echoarea removal tool
URL: http://huskyproject.org
License: GPL
Requires: fidoconf >= 1.9
BuildRequires: huskylib >= 1.9, smapi >= 2.5
BuildRequires: fidoconf-devel >= 1.9
Source: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
hptkill is an echoarea removal tool from the Husky Project.

%prep
%setup -q -n %{name}

%build
make

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_bindir}/*
%{_mandir}/man1/*
