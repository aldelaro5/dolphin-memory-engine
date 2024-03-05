#!/bin/sh
dolphin="/Applications/Dolphin.app/Contents/MacOS/Dolphin"
emphasis='\033[0;31m'

function dolphin_sign
{

    echo "Signing Dolphin Emulator (usually necessary for new installs)"
    printf "Skip? [y/N] "
    read -r skip
    case "$skip" in
        n|N|"") ;;
        *) return ;;
    esac

    printf "Enter location of Dolphin Emulator app (default=$dolphin): "
    read -r dolphin_
    if [ ! -z "$dolphin_" ]; then dolphin="$dolphin_"; fi

    if [ ! -z "$(codesign -d --entitlements :- "$dolphin" 2> /dev/null | grep get-task-allow)" ]; then
        echo "Dolphin is already re-signed!"
        exit 0
    fi

    new_ent="$(mktemp)"
    codesign -d --entitlements :- "$dolphin" 2> /dev/null | sed 's:</dict>:<key>com.apple.security.get-task-allow</key><true/></dict>:' > "$new_ent"
    codesign --remove-signature "$dolphin"
    if [ $? -ne 0 ]; then
        echo "${emphasis}Dolphin signature could not be removed!\033[0m \n\033[1m** You may need to go to System Settings > Privacy & Security > App Management and enable the slider for 'Terminal'. **\033[0m"
        exit 1
    fi

    codesign -s "$certificate" --entitlements "$new_ent" --deep "$dolphin"
    if [ $? -ne 0 ]; then
        echo "Dolphin could not be re-signed!"
        exit 1
    fi
    echo "Success!"

}

printf "${emphasis}Ensure that you have created a code-signing certificate in the "System" keychain (instructions at https://sourceware.org/gdb/wiki/PermissionsDarwin#Create_a_certificate_in_the_System_Keychain).\033[0m\n"
printf "Enter name of code-signing certificate: "
read -r certificate
if [ -z "$(security find-certificate -c "$certificate" | grep System)" ]; then
    echo "That certificate could not be found in the System keychain!"
    exit 1
fi

dolphin_sign

echo
echo "\033[1mIf macOS initially prevents Dolphin from starting, go to System Settings > Privacy & Security > Open Anyway (scroll to the bottom).\033[0m"