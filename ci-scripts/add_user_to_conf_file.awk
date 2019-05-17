BEGIN{lineIdx=0;captureUEDesc=0}
{
    if ($0 ~/UE0/) {
        captureUEDesc = 1
    }
    if (captureUEDesc == 1) {
        captureLine[lineIdx] = $0
        lineIdx = lineIdx + 1
    }
    print $0
}
END {
   for (ueIdx = 1; ueIdx < num_ues; ueIdx++) {
       print ""
       for (k = 0; k < lineIdx; k++) {
           if (captureLine[k] ~/UE0/) {
               mLine = captureLine[k]
               gsub("UE0", "UE"ueIdx, mLine)
               print mLine
           } else {
               if (captureLine[k] ~/MSIN=/) {
                   mLine = captureLine[k]
                   MSIN=sprintf("%08d", 1111+int(ueIdx))
                   gsub("00001111", MSIN, mLine)
                   print mLine
               } else {
                   print captureLine[k]
               }
           }

       }
   }
}
