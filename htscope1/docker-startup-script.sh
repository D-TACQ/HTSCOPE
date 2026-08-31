export MASTER=acq2106_130
export SLAVE=acq2106_133

echo +running docker startup script

echo ++ running python script to generate st.cmd
echo +++ creating 32 int16 channels and 8 int16 spad channels
echo +++ master is $MASTER and slave is $SLAVE

export UUTS="${MASTER} ${SLAVE}"; ./scripts/make_htscope_st.cmd.py \
    --nchan=40 \
    --data32=0 \
    --ndata=1000000 \
    --host=scarp \
    --user=dt100 \
    --output=iocBoot/iochtscope1/st.cmd \
    $UUTS;

echo ++ creating symlinks
cd /root; ln -s /mnt/afhba.0/${MASTER}/000000/0.00 scarp:dt100:acq2106_133; cd -
cd /root; ln -s /mnt/afhba.1/${SLAVE}/000000/1.00 scarp:dt100:acq2106_130; cd -

echo ++ starting IOC and hts wrapper procservs
./init/start_servers

echo +ending docker startup script

sleep infinity
