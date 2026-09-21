#! /bin/sh

TOOLCHAIN_SCRIPT_SHA256="9ed6f97a16914a1e1f822526d1709e6e197d6ba25d14f6a0d02d95fab5817af1"

curl https://github.com/serenial/generic-armv7a-open-embedded/releases/download/OE-SDK-Warrior-1.1/oecore-x86_64-armv7a-vfp-toolchain-nodistro.0.sh -Lo install-tc.sh
echo "${TOOLCHAIN_SCRIPT_SHA256} install-tc.sh" | sha256sum --check --status
chmod +x ./install-tc.sh
sudo ./install-tc.sh -y
