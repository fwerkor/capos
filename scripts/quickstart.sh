sudo bash ./scripts/auto_install_dependencies.sh
./scripts/feeds update -a
./scripts/feeds install -a
echo -e "\e[31mPreparations Done!"
echo -e "Now you can run \e[2mmake menuconfig \e[22mto select your preferred configuration for the toolchain, target system & firmware packages."
echo -e "(Optional) Run \e[2mmake download \e[22mto download sources required to ensure the stability of compiling. "
echo -e "Run \e[2mmake \e[22mto build your firmware. This will download all sources, build the cross-compile toolchain and then cross-compile the GNU/Linux kernel & all chosen applications for your target system."
