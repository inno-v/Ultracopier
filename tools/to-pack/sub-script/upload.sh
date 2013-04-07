#!/bin/bash

if [ "${TEMP_PATH}" = "" ]
then
	exit;
fi
if [ "${ULTRACOPIER_VERSION}" = "" ]
then
        exit;
fi

cd ${TEMP_PATH}/

echo "Move some elements..."
if [ -d ${TEMP_PATH}/doc/ ]
then
	rsync -artu ${TEMP_PATH}/doc/ /home/first-world.info/doc-ultracopier/
	if [ $? -ne 0 ]
	then
		echo 'rsync failed'
	        exit;
	fi
	rm -Rf ${TEMP_PATH}/doc/
fi
if [ -d ${TEMP_PATH}/plugins/ ]
then
	rsync -artu ${TEMP_PATH}/plugins/ /home/first-world.info/files/ultracopier/plugins/
	if [ $? -ne 0 ]
	then
	        echo 'rsync failed'
	        exit;
	fi
	rm -Rf ${TEMP_PATH}/plugins/
fi
mkdir -p /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/*.tar.xz /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/*.zip /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/*-setup.exe /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
mv ${TEMP_PATH}/*.tar.bz2 /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/
echo "Move some elements... done"

echo "Finalise some elements..."
chown lighttpd.lighttpd -Rf /home/first-world.info/doc-ultracopier/
chown lighttpd.lighttpd -Rf /home/first-world.info/files/ultracopier/plugins/
echo "Finalise some elements... done"

echo "Upload to the shop..."
cd /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ && rm -f /home/first-world.info/ultracopier-shop/download/b3cb8c4a421f46366483ce5a0061227d9320adf3 && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/ultracopier-shop/download/b3cb8c4a421f46366483ce5a0061227d9320adf3 ultracopier-ultimate-*-x86-${ULTRACOPIER_VERSION}-setup.exe && mv /home/first-world.info/ultracopier-shop/download/b3cb8c4a421f46366483ce5a0061227d9320adf3.zip /home/first-world.info/ultracopier-shop/download/b3cb8c4a421f46366483ce5a0061227d9320adf3
cd /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ && rm -f /home/first-world.info/ultracopier-shop/download/0f676af18ad9355161e48244ceb5792cf9f1f809 && nice -n 19 ionice -c 3 zip -9 -q /home/first-world.info/ultracopier-shop/download/0f676af18ad9355161e48244ceb5792cf9f1f809 ultracopier-ultimate-*-x86_64-${ULTRACOPIER_VERSION}-setup.exe && mv /home/first-world.info/ultracopier-shop/download/0f676af18ad9355161e48244ceb5792cf9f1f809.zip /home/first-world.info/ultracopier-shop/download/0f676af18ad9355161e48244ceb5792cf9f1f809
cp /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ultracopier-ultimate-linux-x86_64-pc-${ULTRACOPIER_VERSION}.tar.xz /home/first-world.info/ultracopier-shop/download/7678ec69d380cba38205962f6230bcb3dc5d1a21
cp /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ultracopier-ultimate-mac-os-x-${ULTRACOPIER_VERSION}.dmg /home/first-world.info/ultracopier-shop/download/d6382b673f31a42c71101ed642fe69d3b39dba8a

cd /home/first-world.info/files/ultracopier/plugins/Themes/Teracopy/ && nice -n 19 ionice -c 3 tar cpf - *x86_64.urc *x86.urc *mac-os-x.urc *linux-x86_64-pc.urc | nice -n 19 ionice -c 3 xz -z -9 -e > /home/first-world.info/ultracopier-shop/download/161e15b3dfd41a1c4fc265d8d2d856a07e8df559
cd /home/first-world.info/files/ultracopier/plugins/CopyEngine/Rsync/ && nice -n 19 ionice -c 3 tar cpf - *x86_64.urc *x86.urc *mac-os-x.urc *linux-x86_64-pc.urc | nice -n 19 ionice -c 3 xz -z -9 -e > /home/first-world.info/ultracopier-shop/download/7fee8026fb4f7d9bfcb9790dfa0db25a514f79da
cd /home/first-world.info/files/ultracopier/plugins/Themes/Windows/ && nice -n 19 ionice -c 3 tar cpf - *x86_64.urc *x86.urc *mac-os-x.urc *linux-x86_64-pc.urc | nice -n 19 ionice -c 3 xz -z -9 -e > /home/first-world.info/ultracopier-shop/download/59c9fb956fedf4d7a6ef6fe84371882bc5591256
cd /home/first-world.info/files/ultracopier/plugins/Themes/Supercopier/ && nice -n 19 ionice -c 3 tar cpf - *x86_64.urc *x86.urc *mac-os-x.urc *linux-x86_64-pc.urc | nice -n 19 ionice -c 3 xz -z -9 -e > /home/first-world.info/ultracopier-shop/download/c3386f6d227585eb9672fff25b5865208a451cc3

/usr/bin/php /home/first-world.info/ultracopier-shop/update_ultracopier_version.php ${ULTRACOPIER_VERSION}
echo "Upload to the shop... done"

#echo "Clean the ultimate version..."
#rm -f /home/first-world.info/files/ultracopier/${ULTRACOPIER_VERSION}/ultracopier-ultimate*
#echo "Clean the ultimate version... done"

