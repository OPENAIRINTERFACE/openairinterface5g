#!/usr/bin/perl
# This script extracts the ASN1 definition from and TS 38.331 and generates 3 output files that can be processed by asn2wrs
# First download the specification from 3gpp.org as a word document and open it
# Then in "view" menu, select normal or web layout (needed to removed page header and footers)
# Finally save the document as a text file
# Call the script: "perl extract_asn1_from_spec.pl 38331-xxx.txt"
# It should generate: NR-RRC-38331.asn
use warnings;
$input_file = $ARGV[0];
$NR_asn_output_file = "NR-RRC-38331.asn";

sub extract_asn1;

open(INPUT_FILE, "< $input_file") or die "Can not open file $input_file";

while (<INPUT_FILE>) {

  # Process the NR-RRC-Definitions section
  if( m/NR-RRC-Definitions DEFINITIONS AUTOMATIC TAGS ::=/){
    open(OUTPUT_FILE, "> $NR_asn_output_file") or die "Can not open file $NR_asn_output_file";
    syswrite OUTPUT_FILE,"$_ \n";
    syswrite OUTPUT_FILE,"BEGIN\n\n";

    # Get all the text delimited by -- ASN1START and -- ASN1STOP
    extract_asn1();

    syswrite OUTPUT_FILE,"END\n\n";

	  while(<INPUT_FILE>) {
      if( m/PC5-RRC-Definitions DEFINITIONS AUTOMATIC TAGS ::=/){
        syswrite OUTPUT_FILE,"$_ \n";
        syswrite OUTPUT_FILE,"BEGIN\n\n";

        # Get all the text delimited by -- ASN1START and -- ASN1STOP
        extract_asn1();

        syswrite OUTPUT_FILE,"END\n\n";
          
        while(<INPUT_FILE>) {
          if( m/NR-UE-Variables DEFINITIONS AUTOMATIC TAGS ::=/){
            syswrite OUTPUT_FILE,"$_ \n";
            syswrite OUTPUT_FILE,"BEGIN\n\n";

            # Get all the text delimited by -- ASN1START and -- ASN1STOP
            extract_asn1();
            syswrite OUTPUT_FILE,"END\n\n";
            while(<INPUT_FILE>) {
              if( m/NR-Sidelink-Preconf DEFINITIONS AUTOMATIC TAGS ::=/){
                syswrite OUTPUT_FILE,"$_ \n";
                syswrite OUTPUT_FILE,"BEGIN\n\n";

                # Get all the text delimited by -- ASN1START and -- ASN1STOP
                extract_asn1();

                syswrite OUTPUT_FILE,"END\n\n";
                while(<INPUT_FILE>) {
                  if( m/NR-InterNodeDefinitions DEFINITIONS AUTOMATIC TAGS ::=/){
                    syswrite OUTPUT_FILE,"$_ \n";
                    syswrite OUTPUT_FILE,"BEGIN\n\n";

                    # Get all the text delimited by -- ASN1START and -- ASN1STOP
                    extract_asn1();

                    syswrite OUTPUT_FILE,"END\n\n";
                  }
                }
              }
            }
          }
        }
	    }
    }
    close(OUTPUT_FILE);
  }
}

close(INPUT_FILE);

# This subroutine copies the text delimited by -- ASN1START and -- ASN1STOP in INPUT_FILE
# and copies it into OUTPUT_FILE.
# It stops when it meets the keyword "END"
sub extract_asn1 {
  my $line = <INPUT_FILE>;
  my $is_asn1 = 0;

  while(($line ne "END\n") && ($line ne "END\r\n")){
    if ($line =~ m/-- ASN1STOP/) {
      $is_asn1 = 0;
    }
    if ($is_asn1 == 1){
      syswrite OUTPUT_FILE,"$line";
    }
    if ($line =~ m/-- ASN1START/) {
      $is_asn1 = 1;
    }
    $line = <INPUT_FILE>;
  }
}
