BEGIN{lineIdx=0}
{
    captureLine[lineIdx] = $0
    lineIdx = lineIdx + 1
    print $0
}
END{
   for (ueIdx = 1; ueIdx < num_ues; ueIdx++) {
       for (k = 0; k < lineIdx; k++) {
           if (captureLine[k] ~/UserName=/) {
               mLine = captureLine[k]
               MSIN=sprintf("%08d", 1111+int(ueIdx))
               gsub("00001111", MSIN, mLine)
               print mLine
           } else {
               if (captureLine[k] ~/SubscriptionIndex/) {
                   mLine = captureLine[k]
                   MSIN=sprintf("%d", 111+int(ueIdx))
                   gsub("111", MSIN, mLine)
                   print mLine
               } else {
                   print captureLine[k]
               }
           }
       }
   }
}
