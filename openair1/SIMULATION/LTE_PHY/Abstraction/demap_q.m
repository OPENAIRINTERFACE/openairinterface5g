function [q] = demap_q(qq)

if (qq == 1)
               q = -1;
           else
               if(qq == 2)
                   q = 1i;
               else
                   if(qq == 3)
                       q = -1i;
                   else
                       q = 1;
                   end
               end
end

