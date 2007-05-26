language=$1

if [ $language = "English" ]; then
  echo "You cannot load English."
  exit 1
fi

if ! [ -d Localizations/$language ]; then
  echo "You do not have that language available."
  exit 1
fi

if ! [ -d $language.lproj ]; then
  mkdir $language.lproj
fi

if ! [ -d Prferences/General/$language.lproj ]; then
  mkdir Preferences/General/$language.lproj
fi

nibtool -d Localizations/$language/MainUI.strings English.lproj/MainMenu.nib -W $language.lproj/MainMenu.nib
nibtool -d Localizations/$language/OpenURLPanel.strings English.lproj/OpenURLPanel.nib -W $language.lproj/OpenURLPanel.nib
nibtool -d Localizations/$language/PreferencesUI.strings Preferences/General/English.lproj/Preferences.nib -W Preferences/General/$language.lproj/Preferences.nib
cp Localizations/$language/MainProgram.strings $language.lproj/Localizable.strings

