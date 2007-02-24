if ! [ -d Localizations/English ]; then
  mkdir Localizations/English
fi

nibtool -L English.lproj/MainMenu.nib > Localizations/English/MainUI.strings

nibtool -L Preferences/General/English.lproj/Preferences.nib > Localizations/English/PreferencesUI.strings

cp English.lproj/Localizable.strings Localizations/English/MainProgram.strings

