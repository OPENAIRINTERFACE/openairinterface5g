
double median(varArray_t *input) {
  return *(double *)((uint8_t *)(input+1)+(input->size/2)*input->atomSize);
}

double q1(varArray_t *input) {
  return *(double *)((uint8_t *)(input+1)+(input->size/4)*input->atomSize);
}

double q3(varArray_t *input) {
  return *(double *)((uint8_t *)(input+1)+(3*input->size/4)*input->atomSize);
}

void dumpVarArray(varArray_t *input) {
  double *ptr=dataArray(input);
  printf("dumping size=%ld\n", input->size);

  for (int i=0; i < input->size; i++)
    printf("%.1f:", *ptr++);

  printf("\n");
}
void sumUpStats(time_stats_t * res, time_stats_t * src, int lastActive) {
  reset_meas(res);
	for (int i=0; i<RX_NB_TH; i++) {
	  res->diff+=src[i].diff;
	  res->diff_square+=src[i].diff_square;
	  res->trials+=src[i].trials;
	  if (src[i].max > res->max)
	    res->max=src[i].max;
	}
	res->p_time=src[lastActive].p_time;
}
void sumUpStatsSlot(time_stats_t *res, time_stats_t src[RX_NB_TH][2], int lastActive) {
  reset_meas(res);
	for (int i=0; i<RX_NB_TH; i++) {
	  res->diff+=src[i][0].diff+src[i][1].diff;
	  res->diff_square+=src[i][0].diff_square+src[i][1].diff_square;
	  res->trials+=src[i][0].trials+src[i][1].trials;
	  if (src[i][0].max > res->max)
	    res->max=src[i][0].max;
	  if (src[i][1].max > res->max)
	    res->max=src[i][1].max;}
	int last=src[lastActive][0].in < src[lastActive][1].in? 1 : 0 ;
	res->p_time=src[lastActive][last].p_time;
}

double squareRoot(time_stats_t *ptr) {
  double timeBase=1/(1000*get_cpu_freq_GHz());
  return sqrt((double)ptr->diff_square*pow(timeBase,2)/ptr->trials -
              pow((double)ptr->diff/ptr->trials*timeBase,2));
}

void printDistribution(time_stats_t *ptr, varArray_t *sortedList, char *txt) {
  double timeBase=1/(1000*get_cpu_freq_GHz());
  printf("%-43s %6.2f us (%d trials)\n",
         txt,
         (double)ptr->diff/ptr->trials*timeBase,
         ptr->trials);
  printf(" Statistics std=%.2f, median=%.2f, q1=%.2f, q3=%.2f µs (on %ld trials)\n",
         squareRoot(ptr), median(sortedList),q1(sortedList),q3(sortedList), sortedList->size);
}

void printStatIndent(time_stats_t *ptr, char *txt) {
  printf("|__ %-38s %6.2f us (%3d trials)\n",
         txt,
         ptr->trials?inMicroS(ptr->diff/ptr->trials):0,
         ptr->trials);
}

void printStatIndent2(time_stats_t *ptr, char *txt) {
  double timeBase=1/(1000*get_cpu_freq_GHz());
  printf("    |__ %-34s %6.2f us (%3d trials)\n",
         txt,
         ptr->trials?((double)ptr->diff)/ptr->trials*timeBase:0,
	 ptr->trials);
}

void printStatIndent3(time_stats_t *ptr, char *txt) {
  double timeBase=1/(1000*get_cpu_freq_GHz());
  printf("        |__ %-30s %6.2f us (%3d trials)\n",
         txt,
         ptr->trials?((double)ptr->diff)/ptr->trials*timeBase:0,
	 ptr->trials);
}


void logDistribution(FILE* fd, time_stats_t *ptr, varArray_t *sortedList, int dropped) {
  fprintf(fd,"%f;%f;%f;%f;%f;%f;%d;",
	  squareRoot(ptr),
	  (double)ptr->max, *(double*)dataArray(sortedList),
	  median(sortedList),q1(sortedList),q3(sortedList),
	  dropped);
}

struct option * parse_oai_options(paramdef_t *options) {
  int l;

  for(l=0; options[l].optname[0]!=0; l++) {};

  struct option *long_options=calloc(sizeof(struct option),l);

  for(int i=0; options[i].optname[0]!=0; i++) {
    long_options[i].name=options[i].optname;
    long_options[i].has_arg=options[i].paramflags==PARAMFLAG_BOOL?no_argument:required_argument;

    if ( options[i].voidptr)
      switch (options[i].type) {
      case TYPE_INT:
	*options[i].iptr=options[i].defintval;
	break;

      case TYPE_DOUBLE:
	*options[i].dblptr=options[i].defdblval;
	break;

      case TYPE_UINT8:
	*options[i].u8ptr=options[i].defintval;
	break;

      case TYPE_UINT16:
	*options[i].u16ptr=options[i].defintval;
	break;

      default:
	printf("not parsed type for default value %s\n", options[i].optname );
	exit(1);
      }

    continue;
  };
  return long_options;
}

void display_options_values(paramdef_t *options, int verbose) {
  for(paramdef_t * ptr=options; ptr->optname[0]!=0; ptr++) {
    char varText[256]="need specific display";

    if (ptr->voidptr != NULL) {
      if ( (ptr->paramflags & PARAMFLAG_BOOL) )
        strcpy(varText, *(bool *)ptr->iptr ? "True": "False" );
      else  switch (ptr->type) {
          case TYPE_INT:
            sprintf(varText,"%d",*ptr->iptr);
            break;

          case TYPE_DOUBLE:
            sprintf(varText,"%.2f",*ptr->dblptr);
            break;

	case TYPE_UINT8:
	  sprintf(varText,"%d",(int)*ptr->u8ptr);
	  break;

	case TYPE_UINT16:
	  sprintf(varText,"%d",(int)*ptr->u16ptr);
	  break;

	default:
	  printf("not decoded type\n");
	  exit(1);
        }
    }

    printf("--%-20s set to %s\n",ptr->optname, varText);
    if (verbose) printf("%s\n",ptr->helpstr);
  }
}
