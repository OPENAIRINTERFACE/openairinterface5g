BEGIN{max=0;min=10000}
{
    if ($0 ~/Mbits/) {
        split($0,a,"MBytes")
        split(a[2],b)
        if (b[1]>max) {
            max=b[1]
        }
        if (b[1]<min) {
            min=b[1]
        }
     }
}
END{print "Avg Bitrate : " b[1] " Mbits/sec Max Bitrate : " max " Mbits/sec Min Bitrate : " min " Mbits/sec"}
