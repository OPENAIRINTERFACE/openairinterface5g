#include <stdio.h>
#include <NRPPA_MeasurementRequest.h>
#include <NRPPA_ProtocolIE-Field.h>
#include <asn_SEQUENCE_OF.h>
/*
 * This is a custom function which writes the
 * encoded output into some FILE stream.
 */

static int write_out(const void *buffer, size_t size, void *app_key)
{
  FILE *out_fp = app_key;
  size_t wrote;

  wrote = fwrite(buffer, 1, size, out_fp);

  return (wrote == size) ? 0 : -1;
}

int main(int ac, char **av)
{
  NRPPA_MeasurementRequest_t *NRPPA_MeasurementRequest; /* Type to encode */
  asn_enc_rval_t ec; /* Encoder return value */

  /* Allocate the NRPPA_MeasurementRequest_t */
  NRPPA_MeasurementRequest = calloc(1, sizeof(NRPPA_MeasurementRequest_t)); /* not malloc! */
  if (!NRPPA_MeasurementRequest) {
    perror("calloc() failed");
    exit(71); /* better, EX_OSERR */
  }

  NRPPA_MeasurementRequest_IEs_t xyz;
  ASN_SEQUENCE_ADD(&NRPPA_MeasurementRequest->protocolIEs.list, &xyz);

  /* MeasurementRequest ::= SEQUENCE {
protocolIEs  ProtocolIE-Container {{MeasurementRequest-IEs}},
...
} */

  /* Protocl IE container
  id-LMF-Measurement-ID			ProtocolIE-ID ::= 39
  id-RAN-Measurement-ID			ProtocolIE-ID ::= 40
  ID id-TRP-MeasurementRequestList	ProtocolIE-ID ::= 41
  ID id-ReportCharacteristics		ProtocolIE-ID ::= 3
  ID id-MeasurementPeriodicity		ProtocolIE-ID ::= 4
  ID id-TRPMeasurementQuantities		ProtocolIE-ID ::= 52
  */
  //NRPPA_MeasurementRequest->protocolIEs;

  /* Context for parsing across buffer boundaries */
  //NRPPA_MeasurementRequest->_asn_ctx;

  /* BER encode the data if filename is given */
  if (ac < 2) {
    fprintf(stderr, "Specify filename for BER output\n");
  } else {
    const char *filename = av[1];
    FILE *fp = fopen(filename, "wb"); /* for BER output */

    if (!fp) {
      perror(filename);
      exit(71); /* better, EX_OSERR */
    }

    // asn_enc_rval_t der_encode(const struct asn_TYPE_descriptor_s *type_descriptor,
    //                          const void *struct_ptr, /* Structure to be encoded */
    //                          asn_app_consume_bytes_f *consume_bytes_cb,
    //                          void *app_key /* Arbitrary callback argument */);

    /* Encode the NRPPA_MeasurementRequest type as BER (DER) */
    ec = der_encode(&asn_DEF_NRPPA_MeasurementRequest, NRPPA_MeasurementRequest, write_out, fp);

    fclose(fp);
    if (ec.encoded == -1) {
      fprintf(stderr, "Could not encode NRPPA_MeasurementRequest (at %s)\n", ec.failed_type ? ec.failed_type->name : "unknown");
      exit(65); /* better, EX_DATAERR */
    } else {
      fprintf(stderr, "Created %s with BER encoded NRPPA_MeasurementRequest\n", filename);
    }
  }

  /* Also print the constructed NRPPA_MeasurementRequest XER encoded (XML) */
  xer_fprint(stdout, &asn_DEF_NRPPA_MeasurementRequest, NRPPA_MeasurementRequest);

  return 0; /* Encoding finished successfully */
}
