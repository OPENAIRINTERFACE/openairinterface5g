/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

typedef struct {
  int mcc;
  int mnc;
  const char oper_country[100];
  const char oper_short[100];
} plmn_data_t;

static const plmn_data_t plmn_data[] = {
  // Test
  {1,            1,     "OAI Testing","OAI Test"},
  // Abkhazia
  {289,   67,     "Aquafon Abkhazia", ""},
  {289,   68,     "A-Mobile Abkhazia", ""},

  //Afganistan
  {412,   1,    "Afghan Wireless Communication Company","AWCC Afganistan"},
  {412,   20,     "Telecom Development Company Afghanistan Ltd.","Roshan Afganistan"},
  {412,         40,     "MNT Group Afganistan",   "MNT Afganistan"},
  {412,   50,     "Etisalat Afghanistan", "Etisalat Afganistan"},

  //Albania
  {276,   1,    "Albanian Mobile Communications","AMS Albania"},
  {276,   2,    "Vodafone Albania", "Vodafone Albania"},
  {276,   3,    "Eagle Mobile", "Eagle Mobile Albania"},
  {276,   4,    "Plus Communcation", "Plus Communication Albania"},

  //Algeria
  {603,   1,    "ATM Mobilis",  "Mobilis Algeria"},
  {603,   2,    "Orascom Telecom Algerie Spa", "Djezzy Algeria"},
  {603,   3,    "Wataniya Telecom Algerie","Nedjma Algeria"},

  //American Samoa
  {544,   11,     "Blue Sky Communications", "Blue Sky Samoa"},

  //Andorra
  {213,   3,    "Servei De Tele. DAndorra",   "Mobiland Andorra"},

  //Angola
  {631,   2,    "Unitel S.a.r.l.",  "Unitel Angola"},
  {631,   3,    "Movicel",  "Movicel Angola"},
  {631,   4,    "Movicel",  "Movicel Angola"},

  //Anguilla
  {365,   5,    "Mossel Ltd", "Digicel Anguilla"},
  {365,   10,     "Weblinks Limited", "Weblinks Anguilla"},
  {365,   840,    "Cable & Wireless", "Cable & Wireless Anguilla"},

  //Antigua and Barbuda
  {344,   30,     "Antigua Public Utilities Authority",   "APUA Antigua and Barbuda"},
  {344,   50,     "Antigua Wireless Ventures Limited",  "Digicel Antigua and Barbuda"},
  {344,   920,    "Cable & Wireless (Antigua)",   "Lime Antigua"},
  {344,   930,    "Antigua Wireless Ventures Limited",  "Digicel Antigua and Barbuda"},

  //Argentina
  {722,   1,    "Telefonica Móviles Argentina SA",   "Movistar Argentina"},
  {722,   2,    "Nll Holdings",   "Nextel Argentina"},
  {722,   7,    "Telefonica Móviles Argentina SA",   "Movistar Argentina"},
  {722,   10,     "Telefonica Móviles Argentina SA",   "Movistar Argentina"},
  {722,   20,     "Nll Holdings",   "Nextel Argentina"},
  {722,   34,     "Telecom Personal S.A.",  "Personal Argentina"},
  {722,   35,     "Hutchison Telecommunications Argentina S.A.",  "Port-Hable Argentina"},
  {722,   36,     "Telecom Personal S.A",   "Personal Argentina"},
  {722,   40,     "TE.SA.M Argentina S.A",  "Globalstar"},
  {722,   70,     "Telefonica Móviles Argentina SA",   "Movistar Argentina"},
  {722,   310,    "AMX Argentina S.A",  "Claro Argentina"},
  {722,   320,    "AMX Argentina S.A",  "Claro Argentina"},
  {722,   330,    "AMX Argentina S.A",  "Claro Argentina"},
  {722,   340,    "Telecom Personal S.A.",  "Personal Argentina"},
  {722,   341,    "Telecom Personal S.A.", "Personal Argentina"},
  {722,   350,    "Hutchison Telecommunications Argentina S.A.",  "Port-Hable"},
  /*
  Armenia
  {283  1     ArmenTel  Beeline (telecommunications)  Operational
  {283  4     Karabakh Telecom  Karabakh Telecom  Inactive
  {283  5     K Telecom CJSC  VivaCell-MTS  Operational
  283   10    Orange Armenia  Orange  Operational
  283   77    K Telecom   VivaCell-MTS  Operational

  Aruba
  363   1     Servicio di Telecomunicacio di Aruba  SETAR   Operational
  363   2     Digicel   Digicel   Operational

  Australia
  505   1     Telstra Corporation Ltd.  Telstra   Operational
  505   2     Optus Mobile Pty. Ltd.  Optus   Operational
  505   3     Vodafone Network Pty. Ltd.  Vodafone  Operational
  505   4     Department of Defence     Inactive
  505   5     The Ozitel Network Pty. Ltd.  Ozitel  Inactive
  505   6     Hutchison 3G Australia Pty. Ltd.  Hi3G  Operational
  505   8     One.Tel GSM 1800 Pty. Ltd.  One.Tel   Inactive
  505   9     Airnet Commercial Australia Ltd.  Airnet  Inactive
  505   11    Telstra Corporation Ltd.    Inactive
  505   12    Hutchison 3G Australia Pty. Ltd.  Hi3G  Operational
  505   13    Rail Corporation New South Wales  Railcorp  Inactive
  505   14    Telecom New Zealand   AAPT  Operational
  505   15    3GIS Pty Ltd. (Telstra & Hutchison 3G)  3GIS  Inactive
  505   21    TPG Telecom Limited   SOUL  Operational
  505   24    Advanced Communications Technologies Pty. Ltd.    Operational
  505   38    Vodafone Hutchison Australia Proprietary Limited  Crazy John`s  Operational
  505   71    Telstra Corporation Ltd.  Telstra   Operational
  505   72    Telstra Corporation Ltd.  Telstra   Operational
  505   88    Localstar Holding Pty. Ltd.     Inactive
  505   90    Optus Ltd.  Optus   Operational
  505   99    One.Tel GSM 1800 Pty. Ltd.    Inactive

  Austria
  232   1     A1 Telekom Austria  A1  Operational
  232   2     A1 Telekom Austria  A1  Operational
  232   3     T-Mobile  T-Mobile  Operational
  232   5     Orange  Orange  Operational
  232   6       Orange  Operational
  232   7     tele.ring   Tele.ring   Operational
  232   9     A1 Telekom Austria  A1  Operational
  232   10    Hutchison 3G Austria  3 (Drei)  Operational
  232   11    A1 Telekom Austria  Bob   Operational
  232   12    Yesss (Orange)  Yesss   Operational
  232   14    Hutchison 3G Austria  3 (Drei)  Operational
  232   15    Barablu Mobile Ltd  Barablu   Operational
  232   91    ÖBB  GSM-R A   Inactive

  Azerbaijan
  400   1     Azercell Limited Liability Joint Venture  Azercell  Operational
  400   2     Bakcell Limited Liabil ity Company  Bakcell   Operational
  400   3     Catel   FONEX   Inactive
  400   4     Azerfon   Nar Mobile (Azerfon)  Operational

  Bahamas
  364   39    The Bahamas Telecommunications Company Ltd  BaTelCo   Inactive
  364   390     The Bahamas Telecommunications Company Ltd  BaTelCo   Operational

  Bahrain
  426   1     Bahrain Telecommunications Company  Batelco   Operational
  426   2     Zain Bahrain  Zain BH   Operational
  426   4     STC Bahrain   Viva  Operational

  Bangladesh
  470   1     GrameenPhone Ltd  GramenPhone   Operational
  470   2     Axiata Bangladesh Ltd.  Robi  Operational
  470   3     Orascom Telecom Holding   Banglalink  Operational
  470   4     TeleTalk  TeleTalk  Operational
  470   5     Citycell  Citycell  Operational
  470   6     Airtel  Airtel  Operational
  470   7     Aritel  Airtel  Operational

  Barbados
  342   50    Digicel Barbados  DigiCel   Operational
  342   600     Lime (Cable & Wireless)   Lime (Cable & Wireless)   Operational
  342   750     Digicel   DigiCel   Operational
  342   810     Cingular Wireless     Inactive
  342   820     Sunbeach Communications     Inactive

  Belarus
  257   1     MDC Velcom  Velcom  Operational
  257   2     Mobile TeleSystems  MTS   Operational
  257   3     BelCel  DIALLOG   Operational
  257   4     Belarussian Telecommunications Network  life :)   Operational
  257   501       BelCel JV   Operational

  Back to top
  Belgium
  MCC   MNC   Network   Operator or brand name  Status
  206   1     Belgacom Mobile   Proximus  Operational
  206   5     Telenet   Telenet   Operational
  206   6       Lyca Mobile   Operational
  206   10    France Telecom  Mobistar  Operational
  206   20    KPN   Base  Operational

  Back to top
  Belize
  MCC   MNC   Network   Operator or brand name  Status
  702   67    Belize Telemedia  DigiCell  Operational
  702   68    International Telecommunications Ltd. (INTELCO)   IntelCo   Operational
  702   99    SpeedNet Communications Ltd   Smart   Operational

  Benin
  616   0     BBCOM     Inactive
  616   1     Benin Telecoms Mobile   Libercom  Operational
  616   2     Telecel Benin   Moov  Operational
  616   3     Spacetel Benin  MTN   Operational
  616   4     Bell Benin Communications   BBCOM   Operational
  616   5     Glo Communications  Glo   Operational

  Bermuda
  350   1     Cingular GSM 1900   Digicel Bermuda   Operational
  350   2     M3 Wireless   Mobility  Operational
  350   10    Cingular Wireless   Cellular One (Cingular)   Operational
  350   38    Mossel (Digicel)  Digicel   Operational

  Bhutan
  402   11    Bhutan Telecom Ltd  B-Mobile  Operational
  402   17    B-Mobile of Bhutan Telecom    Inactive
  402   77    Tashi InfoComm Limited  TashiCell   Operational

  Bolivia
  736   1     Nuevatel S.A.   Nuevatel  Operational
  736   2     ENTEL S.A.  Entel   Operational
  736   3     Telefonica Celular De Bolivia S.A   Tigo  Operational
  736   20    Entel BOlivia     Inactive

  Bosnia and Herzegovina
  218   3     Public Enterprise Croatian Telecom Ltd.   HT-Eronet   Operational
  218   5     RS Telecommunications JSC Banja Luka  m:tel   Operational
  218   90    BH Telecom  BH Mobile   Operational

  Botswana
  652   1     Mascom Wireless (Pty) Ltd.  Mascom  Operational
  652   2     Orange Botswana (Pty) Ltd.  Orange  Operational
  652   4     Botswana Telecommunications Corporation   BTC Mobile  Operational

  Brazil
  724   0     Nll Holdings, INC.  Nextel  Operational
  724   1     CRT Cellular    Operational
  724   2     Telecom Italia Mobile   TIM   Operational
  724   3     Telecom Italia Mobile   TIM   Operational
  724   4     Telecom Italia Mobile   TIM   Operational
  724   5     Claro   Claro   Operational
  724   6     Vivo S.A  Vivo  Operational
  724   7     CTBC Telecom  CTBC Celular  Operational
  724   8     Telecom Italia Mobile   TIM   Operational
  724   9     Telepar Cel     Inactive
  724   10    Vivo S.A  Vivo  Operational
  724   11    Vivo S.A  Vivo  Operational
  724   12    Americel    Inactive
  724   13    Telesp Cel    Inactive
  724   14    Maxitel BA    Inactive
  724   15    Sercomtel Celular   Sercomtel   Operational
  724   16    Brasil Telecom Celular S.A  Oi  Operational
  724   17    Ceterp Cel    Inactive
  724   18    Norte Brasil Tel    Inactive
  724   19    Telemig Cel     Inactive
  724   21    Telerj Cel    Inactive
  724   23    Vivo S.A  Oi  Operational
  724   24    Amazonia Celular S/A  Oi / Brasil Telecom   Operational
  724   25    Telebrasilia Cel    Inactive
  724   27    Telegoias Cel     Inactive
  724   29    Telemat Cel     Inactive
  724   31    TNL PCS   Oi  Operational
  724   32    CTBC Celular  CTBC Celular  Operational
  724   33    CTBC Celular  CTBC Celular  Operational
  724   34    CTBC Celular  CTBC Celular  Operational
  724   35    Telebahia Cel     Inactive
  724   37    Unicel Do Brasil  Aeiou   Operational
  724   39    NII Holdings Inc  Nextel  Inactive
  724   41    Telpe Cel     Inactive
  724   43    Telepisa Cel    Inactive
  724   45    Telpa Cel     Inactive
  724   47    Telern Cel    Inactive
  724   48    Teleceara Cel     Inactive
  724   51    Telma Cel     Inactive
  724   53    Telepara Cel    Inactive
  724   55    Teleamazon Cel    Inactive
  724   57    Teleamapa Cel     Inactive
  724   59    Telaima Cel     Inactive

  British Virgin Islands
  348   170     Cabel & Wireless (west Indies)  Cabel & Wireless  Operational
  348   570     Caribbean Cellular Telephone, Boatphone Ltd.  CCT Boatphone   Operational
  348   770     Digicel (BVI) Limited   Digicel   Operational

  Brunei Darussalam
  528   1     Jabatan Telekom Brunei    Operational
  528   2     B-Mobile Communications Sdn Bhd   B-Mobile  Operational
  528   11    DST Com   DSTCom  Operational

  Bulgaria
  284   1     M-Tel GSM BG  M-Tel   Operational
  284   3     BTC Mobile  Vivacom   Operational
  284   5     Globul  GLOBUL  Operational
  284   6     BTC Mobile  Vivacom   Operational

  Burkina Faso
  613   1     Onatal  Telmob  Operational
  613   2     Celtel Burkina Faso   Zain  Operational
  613   3     Telcel Faso SA  Telcel Faso   Operational

  Burundi
  642   1     Econet Wireless Burundi PLC   Spacetel  Operational
  642   2     Africell PLC  Africell  Operational
  642   3     Onatel  Onatel  Operational
  642   7     LACELL SU   Smart Mobile  Operational
  642   8     HiTs Telecom  HiTs Telecom  Inactive
  642   82    U-COM Burundi S.A.  U-COM Burundi   Operational

  Cambodia
  456   1     CamGSM  Mobitel   Operational
  456   2     Telekom Malaysia International (Cambodia) Co. Ltd   Hello   Operational
  456   3     S Telecom (reserved)  S Telecom   Operational
  456   4     Cambodia Advance Communications Co. Ltd   QB  Operational
  456   5     Applifone Co. Ltd   Star-Cell   Operational
  456   6     Latelz Co. Ltd  Smart Mobile  Operational
  456   8     Viettel   Mefone  Operational
  456   9     Sotelco Ltd.  Beeline   Operational
  456   11    Excell  Excell  Operational
  456   18    Camshin (Shinawatra)  Mfone   Operational

  Cameroon
  624   1     Mobile Telephone Networks Cameroon  MTN Cameroon  Operational
  624   2     Orange Cameroun   Orange  Operational
  */

  //Canada
  {302,   220,    "Telus Mobility",   "Telus CA"},
  {302,   221,    "Telus Mobility (Unknown)",   "Telus CA"},
  {302,   290,    "Aurtek Wurekess", "Aurtek CA"},
  {302,   320,    "Dave Wireless",  "Mobilicity CA"},
  {302,   350,    "FIRST Networks Operations",  "FIRST CA"},
  {302,   360,    "Telus Mobility",   "MiKE CA"},
  {302,   361,    "Telus Mobility",   "Telus CA"},
  {302,   370,    "Fido Solutions (Rogers Wireless)",   "Fido CA"},
  {302,   380,    "Dryden Mobility",  "DMTS CA"},
  {302,   490,    "Globalive Communications",   "WIND Mobile CA"},
  {302,   500,    "Videotron",  "Videotron CA"},
  {302,   510,    "Videotron",    "Videotron CA"},
  {302,   610,    "Bell Mobility",  "Bell CA"},
  {302,   620,    "ICE Wireless",   "ICE Wireless CA"},
  {302,   640,    "Bell Mobility",  "Bell CA"},
  {302,   651,    "Bell Mobility",  "Bell CA"},
  {302,   652,    "BC Tel Mobility",      "BC Tel CA"},
  {302,   653,    "Telus Mobility",   "Telus"},
  {302,   654,    "Sask Tel Mobility",    "Sask Tel CA"},
  {302,   655,    "MTS Mobility",   "MTS CA"},
  {302,   656,    "Thunder Bay Telephone Mobility",   "TBay CA"},
  {302,   657,    "Telus Mobility",   "Telus CA"},
  {302,   680,    "SaskTel Mobility",   "SaskTel CA"},
  {302,   701,    "MB Tel Mobility",      "MB Tel CA"},
  {302,   702,    "MT&T Mobility (Aliant)", "MT&T CA"},
  {302,   703,    "New Tel Mobility (Aliant", "New Tel CA"},
  {302,   710,    "Globalstar Canada",  "Globalstar CA"},
  {302,   720,    "Rogers Communications", "Rogers Wireless CA"},
  {302,   780,    "SaskTel Mobility",   "SaskTel CA"},
  {302,   880,    "Shared Telus, Bell, and SaskTel",  "Bell / Telus / SaskTel CA"},
  /*
  Cape Verde
  625   1     CVMovel, S.A.   CVMOVEL   Operational
  625   2     T+Telecomunicaçôes  T+  Operational

  Cayman Islands
  346   50    Digicel Cayman Ltd.   Digicel   Operational
  346   140     Cable & Wireless (Cayman) Limited   Cable & Wireless (Lime)   Operational

  Central African Republic
  623   1     Atlantique Telecom Centrafrique SA  MOOV  Operational
  623   2     Telecel Centrafrique (TC)   TC  Operational
  623   3     Orange RCA  Orange  Operational
  623   4     Nationlink Telecom RCA  Nationlink  Operational

  Chad
  622   1     CelTel Tchad SA   Zain  Operational
  622   2     Tchad Mobile  Tawali  Operational
  622   3     Millicom Tchad  Millicom Tchad  Operational
  622   4     Sotel Mobile  Salam   Operational

  Chile
  730   1     Entel PCS Telecomunicaciones S.A.   entel   Operational
  730   2     Telefónica Móvil de Chile   movistar  Operational
  730   3     Claro Chile S.A.  Claro   Operational
  730   4     Centennial Cayman Corp. Chile S.A.  Nextel  Operational
  730   5     VTW S.A.  VTR Móvil  Inactive
  730   9     Centennial Cayman Corp. Chile   Nextel  Operational
  730   10    Entel Telefonia Móvil S.A.   entel   Operational
  730   99    WILL Telefonia  Will  Operational
  */

  //China
  {460,   0,    "China Mobile", "China Mobile"},
  {460,   1,    "China Unicom", "China Unicom"},
  {460,   2,    "Liaoning PPTA","Liaoning China"},
  {460,   3,    "China Unicom CDMA", "China Unicom"},
  {460,   4,    "China Satellite Global Star Network",""},
  {460,   5,    "China Telecom", "China Telecom"},
  {460,   6,    "China Unicom", "China Unicom"},
  {460,   7,    "China Mobile", "China Mobile"},
  {460,   20,     "China Tietong (GSM-R)",""},

  /*
  Colombia
  732   1     Colombia Telecomunicaciones S.A.    Operational
  732   2     Edatel S.A.   Edatel  Operational
  732   20    Emtelsa     Inactive
  732   99    Emcali    Inactive
  732   101     Comcel S.A. Occel S.A./Celcaribe  Comcel  Operational
  732   102     Bellsouth Colombia S.A.   Movistar  Operational
  732   103     Colombia Móvil S.A.  Tigo  Operational
  732   111     Colombia Móvil S.A.  Tigo  Operational
  732   123     Telefónica Móviles Colombia   Movistar  Operational
  732   130     Avantel     Inactive

  Comoros and Mayotte
  654   1     HURI - SNPT     Operational

  Cook Islands
  548   1     Telecom Cook    Operational

  Costa Rica
  712   1     Instituto Costarricense de Electricidad - ICE   ICE   Operational
  712   2     Instituto Costarricense de Electricidad - ICE   ICE   Operational
  712   3     Gruop ICE   ICE   Operational

  Côte d'Ivoire
  612   1     Cora de Comstar     Inactive
  612   2     Moov    Operational
  612   3     Orange    Operational
  612   4     Koz     Operational
  612   5     MTN     Operational
  612   6     OriCel    Operational

  Croatia
  219   1     T-Mobile    Operational
  219   2     Tele2     Operational
  219   10    VIPnet    Operational

  Cuba
  368   1     Empresa de Telecomunicaciones de Cuba, SA   Cubacel   Operational

  Cyprus
  280   0     Areeba Ltd.     Inactive
  280   1     CYTA  Cytamobile-Vodafone   Operational
  280   10    Areeba Ltd  MTN   Operational
  280   20    Primetel PLC  Primetel  Operational

  Czech Rep.
  230   1     T-Mobile Czech Republic a.s.  T-Mobile  Operational
  230   2     Telefónica O2 Czech Republic a.s.  O2  Operational
  230   3     Vodafone Czech Republic a.s.  Vodafone  Operational
  230   4     Mobilkom a.s.   U:fon   Operational
  230   5     Travel Telecomunication s.r.o.    Inactive
  230   6     Osno Telecomunication s.r.o.    Inactive
  230   10    Telefónica O2 Czech Republic a.s.  O2  Operational
  230   98    Sprava Zeleznicni Dopravni Cesty  SŽDC s.o.  Inactive
  230   99    Vodafone Czech Republic a.s. R&D Centre   Vodafone  Operational

  Back to top
  Democratic Rep. of Congo
  MCC   MNC   Network   Operator or brand name  Status
  630   1     Vodacom Congo RDC sprl  Vodacom   Operational
  630   2     Airtel sprl   Airtel  Operational
  630   4       Cellco  Inactive
  630   5     Supercell sprl  Supercell   Operational
  630   10    Libertis Telecom  Libertis  Inactive
  630   86    Orange RDC sarl   Orange  Operational
  630   88    Yozma Timeturns   YTT   Inactive
  630   89    OASIS sprl  Tigo  Operational
  630   90    Africell RDC Sprl   Africell  Operational

  Denmark
  238   1     TDC Mobil APS   TDC   Operational
  238   2     Telenor Denmark   Telenor   Operational
  238   3     End2End     Operational
  238   5     Dansk Beredskapskommunikasjon   Dansk Beredskapskommunikasjon   Inactive
  238   6     H3G APS   3   Operational
  238   7     Mundio Mobile   Mundio Mobile   Operational
  238   8       Nordisk Mobiltelefon  Operational
  238   10    TDC Mobil APS   TDC   Operational
  238   12    Lyca Mobile Denmark Ltd   Lyca  Operational
  238   20    Telia Sonera APS  Telia   Operational
  238   23      Banedanmark   Inactive
  238   30    Telia Sonera APS  Telia   Operational
  238   40      Ericsson Danmark A/S  Inactive
  238   77    Telenor Denmark   Telenor   Operational

  Djibouti
  638   1     Evatis    Operational

  Dominica
  366   20    Cingular Wireless   Digicel   Operational
  366   110     Cable & Wireless Dominica Ltd.    Operational

  Dominican Republic
  370   1     Orange Dominicana, S.A.   Orange  Operational
  370   2     Compañía Dominicana de Teléfonos, C por  Claro   Operational
  370   3     Tricom S.A.   Tricom  Operational
  370   4     Trilogy Dominicana, S.A.  Viva  Operational

  Ecuador
  740   0     Otecel S.A. - Bellsouth   Moviestar   Operational
  740   1     América Móvil   Porta   Operational
  740   2     Telecsa S.A.  Alegro  Operational

  Egypt
  602   1     EEMS - Mobinil  Mobinil   Operational
  602   2     Vodafone Egypt  Vodafone  Operational
  602   3     Etisalat Egypt  Etisalat  Operational

  El Salvador
  706   1     CTE Telecom Personal, S.A. de C.V.  CTW Telecom Personal  Operational
  706   2     Digicel Group   Digicel   Operational
  706   3     Telemovil EL Salvador S.A.  Tigo  Operational
  706   4     Telefónica Móviles El Salvador  movistar  Operational
  706   10    América Móvil   Claro   Operational

  Equatorial Guinea
  627   1     Guinea Ecuatorial de Telecomunicaciones Sociedad Anónima   Orange GQ   Operational
  627   3     HiTs EG.SA  Hits GQ   Operational

  Estonia
  248   1     EMT GSM   EMT   Operational
  248   2     Elisa Eesti   Elisa   Operational
  248   3     Tele 2 Eesti  Tele 2  Operational
  248   4     OY Top Connect    Inactive
  248   5     AS Bravocom Mobiil    Inactive
  248   6     OY ViaTel (UMTS)    Inactive
  248   7     Televõrgu AS     Inactive
  248   71    Siseministeerium (Ministry of Interior)     Inactive

  Ethiopia
  636   1     Ethiopian Telecoms Auth. (ETH MTN)  ETH MTN   Operational

  Falkland Islands
  750   1     Touch   Cable & Wireless  Operational

  Faroe Islands
  288   1     Faroese Telecom - GSM     Operational
  288   2     Vodafone Faroe Islands  Vodafone  Operational

  Fiji
  542   1     Vodafone  Vodafone  Operational
  542   2     Digicel Fiji  Digicel   Operational

  Finland
  244   3     DNA Oy  DNA   Operational
  244   5     Elisa Oyj   Elisa   Operational
  244   7     Nokia test network  Nokia   Inactive
  244   8     Unknown network/operator  Uknown brand  Inactive
  244   9     Finnet Group  Finnet  Operational
  244   10    TDC Oy  TDC   Operational
  244   11    Soumen Erillisverkot Oy   Virve   Inactive
  244   12    DNA Oy  DNA   Operational
  244   14    Alands Mobiltelefon AB  AMT   Operational
  244   15    Samk student network  Samk  Operational
  244   16    Oy Finland Tele2 AB     Inactive
  244   21    Saunalahti  Saunalahti  Operational
  244   29      Scnl Truphone   Operational
  244   41    Saunalahti  Saunalahti  Inactive
  244   91    TeliaSonera Finland Oyj   Sonera  Operational
  */
  //France
  {208,   0,    "France Telecom",   "Orange FR"},
  {208,   1,    "France Telecom",   "Orange FR"},
  {208,   2,    "France Telecom",   "Orange FR"},
  {208,   5,    "Globalstar Europe", ""},
  {208,   6,    "Globalstar Europe", ""},
  {208,   7,    "Globalstar Europe", ""},
  {208,   10,     "Vivendi",  "SFR FR"},
  {208,   11,     "Vivendi",  "SFR FR"},
  {208,   13,     "Vivendi",  "SFR FR"},
  {208,   14,     "Iliad",  "Free Mobile FR"},
  {208,   15,     "Iliad",  "Free Mobile FR"},
  {208,   20,     "Bouygues Telecom",   "Bouygues FR"},
  {208,   21,     "Bouygues Telecom",   "Bouygues FR"},
  {208,   88,     "Bouygues Telecom (Zones Blanches)",  "Bouygues FR"},
  /*
  French Guiana
  742   1     Orange Caribe French Guiana   Orange Caribe French Guiana   Inactive
  742   20    Digicel French Guiana   Digicel   Operational

  French Polynesia
  547   0     Digicel Antilles Francaises Guyane    Inactive
  547   1     Orange Caraibe Mobiles    Inactive
  547   2     Outremer Telecom    Inactive
  547   3     Saint Martin et Saint Barthelemy Telcell Sarl     Inactive
  547   8     AMIGO GSM     Inactive
  547   20    Tikiphone     Operational

  Gabon
  628   1     Gabon Telecom & Libertis S.A.   Libertis  Operational
  628   2     Atlantique Télécom (Etisalat Group)   Moov  Operational
  628   3     Celtel Gabon S.A.   Airtel  Operational
  628   4     USAN Gabon S.A.   Azur  Operational
  628   5     Réseau de l’Administration Gabonaise   RAG   Operational

  Gambia
  607   1     Gamcel  Gamcel  Operational
  607   2     Africell  Africell  Operational
  607   3     Comium Services Ltd   Comium  Operational
  607   4     QCell Gambia  QCell   Operational

  Georgia
  282   1     Geocell Ltd.  Geocell   Operational
  282   2     Magti GSM Ltd.  MagtiCom  Operational
  282   3     Iberiatel Ltd.  Iberiatel   Operational
  282   4     Beeline   Beeline   Operational
  282   5     Sliknet   SLINKNET  Operational
  282   67    Aquafon     Operational
  282   88    A-Mobile    Inactive
  */

  //Germany
  {262,   1,    "T-Mobile Deutschland GmbH",  "T-Mobile D"},
  {262,   2,    "Vodafone D2 GmbH",   "Vodafone D"},
  {262,   3,    "E-Plus Mobilfunk GmbH & Co. KG",   "E-plus D"},
  {262,   4,    "Vodafone D2 GmbH",   "Vodafone D (Reserved)"},
  {262,   5,    "E-Plus Mobilfunk GmbH & Co. KG",   "E-Plus D (Reserved)"},
  {262,   6,    "T-Mobile Deutschland GmbH",  "T-Mobile D (Reserved)"},
  {262,   7,    "O2 (Germany) GmbH & Co. OHG",  "O2 D"},
  {262,   8,    "O2 (Germany) GmbH & Co. OHG",  "O2 D"},
  {262,   9,    "Vodafone D2 GmbH", "Vodafone D"},
  {262,   10,     "Arcor AG & Co. (GSM-R)", ""},
  {262,   11,     "O2 (Germany) GmbH & Co. OHG",  "O2 D (RESERVED)"},
  {262,   12,     "Dolphin Telecom (Deutschland) GmbH", "Dolphin D"},
  {262,   13,     "Mobilcom Multimedia GmbH", ""},
  {262,   14,     "Group 3G UMTS GmbH (Quam)",""},
  {262,   15,     "Airdata AG",   "Airdata D"},
  {262,   16,     "MVNE (E-plus)",  "Vistream D"},
  {262,   17,     "Ring Mobilfunk",   "Ring Mobilfunk D"},
  {262,   20,     "E-Plus",   "OnePhone D"},
  {262,   43,     "Lyca Mobile",  "Lyca D"},
  {262,   60,     "DB Telematik (GSM-R)", ""},
  {262,   76,     "Siemens AG",""},
  {262,   77,     "E-Plus Mobilfunk GmbH & Co. KG","E-Plus D"},
  {262,   901,    "Debitel AG",   "Debitel D"},
  /*
  Ghana
  620   1     MTN Group   MTN   Operational
  620   2     Vodafone Group  Vodafone  Operational
  620   3     Millicom Ghana  tiGO  Operational
  620   4     Kasapa Telecom Ltd.   Expresso  Operational
  620   6     Airtel  Airtel  Operational
  620   997     Glo Mobile Ghana  Glo Mobile  Operational

  Gibraltar
  266   1     Gibtelecom GSM  GibTel  Operational
  266   6     CTS Gibraltar   CTS Mobile  Operational
  266   9     Cloud9 Mobile Communications    Inactive
  */

  //Greece
  {202,   1,    "Cosmote Mobile Teelecommunications S.A.",  "Cosmote G"},
  {202,   5,    "Vodafone - Panafon",   "Vodafone G"},
  {202,   9,    "Wind Hellas Telecommunications S.A.",  "Wind G"},
  {202,   10,     "Wind Hellas Telecommunications S.A.",  "Wind G"},
  /*
  Greenland
  290   1     Tele Greenland    Operational

  Grenada
  352   30    Digicel   Digicel   Operational
  352   110     Cable & Wireless Grenada Ltd.   Cable & Wireless  Operational

  Guam (US)
  535   47    Docomo Pacific Inc    Operational

  Guatemala
  704   1     Servicios de Comunicaciones Personales Inalámbricas, S.A.  Claro   Operational
  704   2     Comunicaciones Celulares S.A.   Comcel / Tigo   Operational
  704   3     Telefónica Centroamérica Guatemala S.A.   Movistar  Operational

  Guinea
  611   1     Orange  Orange S.A.   Operational
  611   2     Sotelgui Lagui  Sotelgui  Operational
  611   3     INTERCEL Guinée  Telecel Guinee  Operational
  611   4     Areeba Guinea   MTN   Operational
  611   5     Cellcom Guinée SA  Cellcom   Operational

  Guinea-Bissau
  632   1     Guinétel S.A.    Inactive
  632   2     Spacetel Guiné-Bissau S.A.     Operational
  632   3     Orange  Orange  Operational
  632   7     Guinetel (Telecel Guinee SARL)    Inactive

  Guyana
  738   1     U-Mobile (Cellular) Inc.  Digicel   Operational
  738   2     Guyana Telephone & Telegraph Co.  GT&T Cellink Plus   Operational

  Haiti
  372   1     Communication Cellulaire dHaiti SA  Voila   Operational
  372   2     Unigestion Holding S.A  Digicel   Operational
  372   3     Telecommunication S.A   NATCOM  Operational
  372   50    Unigestion Holding S.A  Digicel   Operational

  Honduras
  708   1     Servicios de Comunicaciones de Honduras S.A. de C.V.  Claro   Operational
  708   2     Celtel / Tigo   Tigo  Operational
  708   30    Empresa Hondurena de Telecomunicaciones   Hondutel  Operational
  708   40    Digicel de Honduras   DIGICEL   Operational

  Hongkong, China
  454   0     CSL Ltd   1O1O / One2Free   Operational
  454   1     CITIC Telecom 1616    Operational
  454   2     CSL Ltd   CSL   Operational
  454   3     Hutchison Telecom   3 (3G)  Operational
  454   4     Hutchison Telecom   3 (2G)  Operational
  454   5     Hutchison Telecom   3 (CDMA)  Operational
  454   6     SmarTone Mobile Communications Ltd  SmarTone-Vodafone   Operational
  454   7     China Unicom (Hong Kong) Ltd    Operational
  454   8     Trident Telecom     Operational
  454   9     China Motion Telecom    Operational
  454   10    CSL Ltd   New World Mobility  Operational
  454   11    China-Hong Kong Telecom     Operational
  454   12    China Mobile Hong Kong Company Ltd  CMCC HK   Operational
  454   14    Hutchison Telecom     Operational
  454   15    3G Radio System/SMT3G     Operational
  454   16    PCCW Ltd  PCCW Mobile   Operational
  454   17    SmarTone Mobile Communications Limited  SmarTone Mobile Communications Limited  Operational
  454   18    CSL Ltd   CSL   Operational
  454   19    PCCW Ltd  PCCW Mobile   Operational
  454   22      SmarTone Mobile (P-Plus)  Operational
  454   29    PCCW Ltd  PCCW Mobile   Operational
  454   40    shared by private TETRA systems     Inactive
  454   47    Hong Kong Police Force - TETRA systems    Inactive

  Hungary
  216   1     Telenor Magyarország Zrt.  Telenor   Operational
  216   30    Magyar Telekom Plc  T-Mobile  Operational
  216   70    Vodafone Magyarország Zrt.   Vodafone  Operational

  Iceland
  274   1     Iceland Telecom Ltd.  Siminn  Operational
  274   2     Vodafone  Vodafone  Operational
  274   3     Islandssimi GSM ehf   Vodafone  Operational
  274   4     IMC Islande ehf   Viking  Operational
  274   6     09 Mobile     Operational
  274   7     IceCell ehf   IceCell   Operational
  274   11    Nova ehf  Nova  Operational

  India
  404   0     Sistema Shyam (Rajasthan)   Sistema Shyam   Operational
  404   1     Vodafone (Haryana)  Vodafone  Operational
  405   1     Reliance Telecom (Andhra Pradesh)   Reliance Telecom  Operational
  404   2     Bharti Airtel Ltd (Punjab)  Airtel  Operational
  404   3     Bharti Airtel Ltd (Himachal Pradesh)  Airtel  Operational
  405   3     Reliance Telecom (Bihar)  Reliance Telecom  Operational
  405   4     Reliance Telecom (Chennai)  Reliance Telecom  Operational
  404   4     Idea (Delhi)  Idea  Operational
  405   5     Reliance Telecom (Delhi)  Reliance Telecom  Operational
  404   5     Vodafone (Gujarat)  Vodafone  Operational
  405   6     Reliance Telecom (Gujarat)  Reliance Telecom  Operational
  405   7     Reliance Telecom (Haryana)  Reliance Telecom  Operational
  404   7     Idea (Andhra Pradesh)   Idea  Operational
  405   8     Reliance Telecom (Himachal Pradesh)   Reliance Telecom  Operational
  405   9     Reliance Telecom (Jammu & Kashmir)  Reliance Telecom  Operational
  404   9     Reliance Telecom (Assam)  Reliance Telecom  Operational
  405   10    Reliance Telecom (Karnataka)  Reliance Telecom  Operational
  404   10    Bharti Airtel Ltd (Delhi)   Airtel  Operational
  405   11    Reliance Telecom (Kerala)   Reliance Telecom  Operational
  404   11    Vodafone (Delhi)  Vodafone  Operational
  405   12    Reliance Telecom (Kolkata)  Reliance Telecom  Operational
  404   12    Idea (Haryana)  Idea  Operational
  404   13    Vodafone (Andhra Pradesh)   Vodafone  Operational
  405   13    Reliance Telecom (Maharashtra)  Reliance Telecom  Operational
  405   14    Reliance Telecom (Madhya Pradesh)   Reliance Telecom  Operational
  404   14    Idea (Punjab)   Idea  Operational
  405   15    Reliance Telecom (Mumbai)   Reliance Telecom  Operational
  404   15    Vodafone (Uttar Pradesh (East)  Vodafone  Operational
  404   16    Bharti Airtel Ltd (North East)  Airtel  Operational
  404   17    Aircel (West Bengal)  Aircel  Operational
  405   17    Reliance Telecom (Orissa)   Reliance Telecom  Operational
  405   18    Reliance Telecom (Punjab)   Reliance Telecom  Operational
  404   18    Reliance Telecom (Himachal Pradesh)   Reliance Telecom  Operational
  405   19    Reliance Telecom (Rajasthan)  Reliance Telecom  Operational
  404   19    Idea (Kerala)   Idea  Operational
  405   20    Reliance Telecom (Tamilnadu)  Reliance Telecom  Operational
  404   20    Vodafone (Mumbai)   Vodafone  Operational
  405   21    Reliance Telecom (Uttar Pradesh (East)  Reliance Telecom  Operational
  404   21    LOOP (Mumbai)   LOOP  Operational
  404   22    Idea (Maharashtra)  Idea  Operational
  405   22    Reliance Telecom (Uttar Pradesh (West)  Reliance Telecom  Operational
  405   23    Reliance Telecom (West Bengal)  Reliance Telecom  Operational
  404   24    Idea (Gujarat)  Idea  Operational
  405   24    HFCL INFOTEL (Punjab)   HFCL INFOTEL  Operational
  404   25    Aircel (Bihar)  Aircel  Operational
  405   25    TATA Teleservices (Andhra Pradesh)  TATA Teleservices   Operational
  405   26    TATA Teleservices (Assam)   TATA Teleservices   Operational
  405   27    TATA Teleservices (Bihar)   TATA Teleservices   Operational
  404   27    Vodafone (Maharashtra)  Vodafone  Operational
  405   28    TATA Teleservices (Chennai)   TATA Teleservices   Operational
  404   28    Aircel (Orissa)   Aircel  Operational
  404   29    Aircel (Assam)  Aircel  Operational
  405   29    TATA Teleservices (Delhi)   TATA Teleservices   Operational
  405   30    TATA Teleservices (Gujarat)   TATA Teleservices   Operational
  404   30    Vodafone (Kolkata)  Vodafone  Operational
  405   31    TATA Teleservices (Haryana)   TATA Teleservices   Operational
  404   31    Bharti Airtel Ltd (Kolkata)   Airtel  Operational
  405   32    TATA Teleservices (Himachal Pradesh)  TATA Teleservices   Operational
  405   33    TATA Teleservices (Jammu & Kashmir)   TATA Teleservices   Operational
  404   33    Aircel (North East)   Aircel  Operational
  404   34    BSNL (Haryana)  BSNL  Operational
  405   34    TATA Teleservices (Karnataka)   TATA Teleservices   Operational
  405   35    TATA Teleservices (Kerala)  TATA Teleservices   Operational
  404   35    Aircel (Himachal Pradesh)   Aircel  Operational
  405   36    TATA Teleservices (Kolkata)   TATA Teleservices   Operational
  404   36    Reliance Telecom (Bihar)  Reliance Telecom  Operational
  405   37    TATA Teleservices (Maharashtra)   TATA Teleservices   Operational
  404   37    Aircel (Jammu & Kashmir)  Aircel  Operational
  404   38    BSNL (Assam)  BSNL  Operational
  405   38    TATA Teleservices (Madhya Pradesh)  TATA Teleservices   Operational
  405   39    TATA Teleservices (Mumbai)  TATA Teleservices   Operational
  405   40    TATA Teleservices (North East)  TATA Teleservices   Operational
  404   40    Bharti Airtel Ltd (Chennai)   Airtel  Operational
  404   41    Aircel (Chennai)  Aircel  Operational
  405   41    TATA Teleservices (Orissa)  TATA Teleservices   Operational
  405   42    TATA Teleservices (Punjab)  TATA Teleservices   Operational
  404   42    Aircel (Tamilnadu)  Aircel  Operational
  405   43    TATA Teleservices (Rajasthan)   TATA Teleservices   Operational
  404   43    Vodafone (Tamilnadu)  Vodafone  Operational
  405   44    TATA Teleservices (Tamilnadu)   TATA Teleservices   Operational
  404   44    Idea (Karnataka)  Idea  Operational
  404   45    Bharti Airtel Ltd (Karnataka)   Airtel  Operational
  405   45    TATA Teleservices (Uttar Pradesh (East)   TATA Teleservices   Operational
  405   46    TATA Teleservices (Uttar Pradesh (West)   TATA Teleservices   Operational
  404   46    Vodafone (Kerala)   Vodafone  Operational
  405   47    TATA Teleservices (West Bengal)   TATA Teleservices   Operational
  405   48    INDIAN RAILWAYS GSM-R (ALL CIRCLES)   Indian Raylways   Operational
  404   49    Bharti Airtel Ltd (Andhra Pradesh)  Airtel  Operational
  404   50    Reliance Telecom (North East)   Reliance Telecom  Operational
  405   51    Bharti Airtel Ltd (West Bengal)   Airtel  Operational
  404   51    BSNL (Himachal Pradesh)   BSNL  Operational
  405   52    Bharti Airtel Ltd (Bihar)   Airtel  Operational
  404   52    Reliance Telecom (Orissa)   Reliance Telecom  Operational
  405   53    Bharti Airtel Ltd (Orissa)  Airtel  Operational
  404   53    BSNL (Punjab)   BSNL  Operational
  404   54    BSNL (Uttar Pradesh (West)  BSNL  Operational
  405   54    Bharti Airtel Ltd (Uttar Pradesh (East)   Airtel  Operational
  405   55    Bharti Airtel Ltd (Jammu & Kashmir)   Airtel  Operational
  404   55    BSNL (Uttar Pradesh (East)  BSNL  Operational
  404   56    Idea (Uttar Pradesh (West)  Idea  Operational
  405   56    Bharti Airtel Ltd (Assam)   Airtel  Operational
  404   57    BSNL (Gujarat)  BSNL  Operational
  404   58    BSNL (Madhya Pradesh)   BSNL  Operational
  404   59    BSNL (Rajasthan)  BSNL  Operational
  404   60    Vodafone (Rajasthan)  Vodafone  Operational
  404   62    BSNL (Jammu & Kashmir)  BSNL  Operational
  404   64    BSNL (Chennai)  BSNL  Operational
  404   66    BSNL (Maharashtra)  BSNL  Operational
  405   66    Vodafone (Uttar Pradesh (West)  Vodafone  Operational
  405   67    Vodafone (West Bengal)  Vodafone  Operational
  404   67    Reliance Telecom (Madhya Pradesh)   Reliance Telecom  Operational
  404   68    MTNL (Delhi)  Dolphin   Operational
  404   69    MTNL (Mumbai)   Dolphin   Operational
  405   70    Idea (Bihar)  Idea  Operational
  404   70    Bharti Airtel Ltd (Rajasthan)   Airtel  Operational
  404   71    BSNL (Karnataka)  BSNL  Operational
  404   72    BSNL (Kerala)   BSNL  Operational
  404   73    BSNL (Andhra Pradesh)   BSNL  Operational
  404   74    BSNL (West Bengal)  BSNL  Operational
  404   75    BSNL (Bihar)  BSNL  Operational
  404   76    BSNL (Orissa)   BSNL  Operational
  404   77    BSNL (North East)   BSNL  Operational
  404   78    Idea (Madhya Pradesh)   Idea  Operational
  404   79    BSNL (Andaman Nicobar)  BSNL  Operational
  404   80    BSNL (Tamilnadu)  BSNL  Operational
  404   81    BSNL (Kolkata)  BSNL  Operational
  404   82    Idea (Himachal Pradesh)   Idea  Operational
  404   83    Reliance Telecom (Kolkata)  Reliance Telecom  Operational
  404   84    Vodafone (Chennai)  Vodafone  Operational
  404   85    Reliance Telecom (West Bengal)  Reliance Telecom  Operational
  404   86    Vodafone (Karnataka)  Vodafone  Operational
  404   87    Idea (Rajasthan)  Idea  Operational
  404   88    Vodafone (Punjab)   Vodafone  Operational
  404   89    Idea (Uttar Pradesh (East)  Idea  Operational
  404   90    Bharti Airtel Ltd (Maharashtra)   Airtel  Operational
  404   91    Aircel (Kolkata)  Aircel  Operational
  404   92    Bharti Airtel Ltd (Mumbai)  Airtel  Operational
  404   93    Bharti Airtel Ltd (Madhya Pradesh)  Airtel  Operational
  404   94    Bharti Airtel Ltd (Tamilnadu)   Airtel  Operational
  404   95    Bharti Airtel Ltd (Kerala)  Airtel  Operational
  404   96    Bharti Airtel Ltd (Haryana)   Airtel  Operational
  404   97    Bharti Airtel Ltd (Uttar Pradesh (West)   Airtel  Operational
  404   98    Bharti Airtel Ltd (Gujarat)   Airtel  Operational
  405   750     Vodafone (Jammu & Kashmir)  Vodafone  Operational
  405   751     Vodafone (Assam)  Vodafone  Operational
  405   752     Vodafone (Bihar)  Vodafone  Operational
  405   753     Vodafone (Orissa)   Vodafone  Operational
  405   754     Vodafone (Himachal Pradesh)   Vodafone  Operational
  405   755     Vodafone (North East)   Vodafone  Operational
  405   756     Vodafone (Madhya Pradesh)   Vodafone  Operational
  405   799     Idea (Mumbai)   Idea  Operational
  405   800     Aircel (Delhi)  Aircel  Operational
  405   801     Aircel (Andhra Pradesh)   Aircel  Operational
  405   802     Aircel (Gujarat)  Aircel  Operational
  405   803     Aircel (Karnataka)  Aircel  Operational
  405   804     Aircel (Maharashtra)  Aircel  Operational
  405   805     Aircel (Mumbai)   Aircel  Operational
  405   806     Aircel (Rajasthan)  Aircel  Operational
  405   807     Aircel (Haryana)  Aircel  Operational
  405   808     Aircel (Madhya Pradesh)   Aircel  Operational
  405   809     Aircel (Kerala)   Aircel  Operational
  405   810     Aircel (Uttar Pradesh (East)  Aircel  Operational
  405   811     Aircel (Uttar Pradesh (West)  Aircel  Operational
  405   812     Aircel (Punjab)   Aircel  Operational
  405   813     Telenor Unitech (Haryana)   Uninor  Operational
  405   814     Telenor Unitech (Himachal Pradesh)  Uninor  Operational
  405   815     Telenor Unitech (Jammu & Kashmir)   Uninor  Operational
  405   816     Telenor Unitech (Punjab)  Uninor  Operational
  405   817     Telenor Unitech (Rajasthan)   Uninor  Operational
  405   818     Telenor Unitech (Uttar Pradesh (West)   Uninor  Operational
  405   819     Telenor Unitech (Andhra Pradesh)  Uninor  Operational
  405   820     Telenor Unitech (Karnataka)   Uninor  Operational
  405   821     Telenor Unitech (Kerala)  Uninor  Operational
  405   822     Telenor Unitech (Kolkata)   Uninor  Operational
  405   823     Videocon (Andhra Pradesh)   Videocon  Operational
  405   824     Videocon (Assam)  Videocon  Operational
  405   825     Videocon (Bihar)  Videocon  Operational
  405   826     Videocon (Delhi)  Videocon  Operational
  405   827     Videocon (Gujarat)  Videocon  Operational
  405   828     Videocon (Haryana)  Videocon  Operational
  405   829     Videocon (Himachal Pradesh)   Videocon  Operational
  405   830     Videocon (Jammu & Kashmir)  Videocon  Operational
  405   831     Videocon (Karnataka)  Videocon  Operational
  405   832     Videocon (Kerala)   Videocon  Operational
  405   833     Videocon (Kolkata)  Videocon  Operational
  405   834     Videocon (Madhya Pradesh)   Videocon  Operational
  405   835     Videocon (Maharashtra)  Videocon  Operational
  405   836     Videocon (Mumbai)   Videocon  Operational
  405   837     Videocon (North East)   Videocon  Operational
  405   838     Videocon (Orissa)   Videocon  Operational
  405   839     Videocon (Rajasthan)  Videocon  Operational
  405   840     Videocon (Tamilnadu)  Videocon  Operational
  405   841     Videocon (Uttar Pradesh (East)  Videocon  Operational
  405   842     Videocon (Uttar Pradesh (West)  Videocon  Operational
  405   843     Videocon (West Bengal)  Videocon  Operational
  405   844     Telenor Unitech (Delhi)   Uninor  Operational
  405   845     Idea (Assam)  Idea  Operational
  405   846     Idea (Jammu & Kashmir)  Idea  Operational
  405   848     Idea (Kolkata)  Idea  Operational
  405   849     Idea (North East)   Idea  Operational
  405   850     Idea (Orissa)   Idea  Operational
  405   852     Idea (Tamilnadu)  Idea  Operational
  405   853     Idea (West Bengal)  Idea  Operational
  405   854     LOOP (Andhra Pradesh)   LOOP  Operational
  405   855     LOOP (Assam)  LOOP  Operational
  405   856     LOOP (Bihar)  LOOP  Operational
  405   857     LOOP (Delhi)  LOOP  Operational
  405   858     LOOP (Gujarat)  LOOP  Operational
  405   859     LOOP (Haryana)  LOOP  Operational
  405   860     LOOP (Himachal Pradesh)   LOOP  Operational
  405   861     LOOP (Jammu & Kashmir)  LOOP  Operational
  405   862     LOOP (Karnataka)  LOOP  Operational
  405   863     LOOP (Kerala)   LOOP  Operational
  405   864     LOOP (Kolkata)  LOOP  Operational
  405   865     LOOP (Madhya Pradesh)   LOOP  Operational
  405   866     LOOP (Maharashtra)  LOOP  Operational
  405   867     LOOP (North East)   LOOP  Operational
  405   868     LOOP (Orissa)   LOOP  Operational
  405   869     LOOP (Punjab)   LOOP  Operational
  405   870     LOOP (Rajasthan)  LOOP  Operational
  405   871     LOOP (Tamilnadu)  LOOP  Operational
  405   872     LOOP (Uttar Pradesh (East)  LOOP  Operational
  405   873     LOOP (Uttar Pradesh (West)  LOOP  Operational
  405   874     LOOP (West Bengal)  LOOP  Operational
  405   875     Telenor Unitech (Assam)   Uninor  Operational
  405   876     Telenor Unitech (Bihar)   Uninor  Operational
  405   877     Telenor Unitech (North East)  Uninor  Operational
  405   878     Telenor Unitech (Orissa)  Uninor  Operational
  405   879     Telenor Unitech (Uttar Pradesh (East)   Uninor  Operational
  405   880     Telenor Unitech (West Bengal)   Uninor  Operational
  405   881     S TEL (Assam)   S TEL   Operational
  405   882     S TEL (Bihar)   S TEL   Operational
  405   883     S TEL (Himachal Pradesh)  S TEL   Operational
  405   884     S TEL (Jammu & Kashmir)   S TEL   Operational
  405   885     S TEL (North East)  S TEL   Operational
  405   886     S TEL (Orissa)  S TEL   Operational
  405   887     Sistema Shyam (Andhra Pradesh)  Sistema Shyam   Operational
  405   888     Sistema Shyam (Assam)   Sistema Shyam   Operational
  405   889     Sistema Shyam (Bihar)   Sistema Shyam   Operational
  405   890     Sistema Shyam (Delhi)   Sistema Shyam   Operational
  405   891     Sistema Shyam (Gujarat)   Sistema Shyam   Operational
  405   892     Sistema Shyam (Haryana)   Sistema Shyam   Operational
  405   893     Sistema Shyam (Himachal Pradesh)  Sistema Shyam   Operational
  405   894     Sistema Shyam (Jammu & Kashmir)   Sistema Shyam   Operational
  405   895     Sistema Shyam (Karnataka)   Sistema Shyam   Operational
  405   896     Sistema Shyam (Kerala)  Sistema Shyam   Operational
  405   897     Sistema Shyam (Kolkata)   Sistema Shyam   Operational
  405   898     Sistema Shyam (Madhya Pradesh)  Sistema Shyam   Operational
  405   899     Sistema Shyam (Maharashtra)   Sistema Shyam   Operational
  405   900     Sistema Shyam (Mumbai)  Sistema Shyam   Operational
  405   901     Sistema Shyam (North East)  Sistema Shyam   Operational
  405   902     Sistema Shyam (Orissa)  Sistema Shyam   Operational
  405   903     Sistema Shyam (Punjab)  Sistema Shyam   Operational
  405   904     Sistema Shyam (Tamilnadu)   Sistema Shyam   Operational
  405   905     Sistema Shyam (Uttar Pradesh (East)   Sistema Shyam   Operational
  405   906     Sistema Shyam (Uttar Pradesh (West)   Sistema Shyam   Operational
  405   907     Sistema Shyam (West Bengal)   Sistema Shyam   Operational
  405   912     Etisalat DB (Andhra Pradesh)  Etisalat DB   Operational
  405   913     Etisalat DB (Delhi)   Etisalat DB   Operational
  405   914     Etisalat DB (Gujarat)   Etisalat DB   Operational
  405   915     Etisalat DB (Haryana)   Etisalat DB   Operational
  405   916     Etisalat DB (Karnataka)   Etisalat DB   Operational
  405   917     Etisalat DB (Kerala)  Etisalat DB   Operational
  405   918     Etisalat DB (Maharashtra)   Etisalat DB   Operational
  405   919     Etisalat DB (Mumbai)  Etisalat DB   Operational
  405   920     Etisalat DB (Punjab)  Etisalat DB   Operational
  405   921     Etisalat DB (Rajasthan)   Etisalat DB   Operational
  405   922     Etisalat DB (Tamilnadu)   Etisalat DB   Operational
  405   923     Etisalat DB (Uttar Pradesh (East)   Etisalat DB   Operational
  405   924     Etisalat DB (Uttar Pradesh (West)   Etisalat DB   Operational
  405   925     Telenor Unitech (Tamilnadu)   Uninor  Operational
  405   926     Telenor Unitech (Mumbai)  Uninor  Operational
  405   927     Telenor Unitech (Gujarat)   Uninor  Operational
  405   928     Telenor Unitech (Madhya Pradesh)  Uninor  Operational
  405   929     Telenor Unitech (Maharashtra)   Uninor  Operational
  405   930     Etisalat DB (Bihar)   Etisalat DB   Operational
  405   931     Etisalat DB (Madhya Pradesh)  Etisalat DB   Operational
  405   932     Videocon (Punjab)   Videocon  Operational

  Indonesia
  510   0     PT Pasifik Satelit Nusantara (ACeS)   PSN   Operational
  510   1     PT Indonesian Satellite Corporation Tbk (INDOSAT)   INDOSAT   Operational
  510   3     PT Indosat TBK  StarOne   Inactive
  510   7     PT Telkom   TelkomFlexi   Inactive
  510   8     PT Natrindo Telepon Seluler   AXIS  Operational
  510   9     PT Smart Telecom  SMART   Operational
  510   10    PT Telekomunikasi Selular   Telkomsel   Operational
  510   11    PT XL Axiata Tbk  XL  Operational
  510   20    Pt Telcom Indonesia TBK   TelkomMobile  Inactive
  510   21    PT Indonesian Satelite Corporation Tbk (Indosat)  IM3   Inactive
  510   27    PT Sampoerna Telekomunikasi Indonesia   Ceria   Operational
  510   28    PT Mobile-8 Telecom   Fren/Hepi   Operational
  510   89    PT Hutchison CP Telecommunications  3   Operational
  510   99    PT Bakrie Telecom   Esia  Inactive

  International
  901   1     ICO Satellite Management  ICO   Operational
  901   3     Iridium   Iridium   Operational
  901   4     Globalstar  Globalstar  Operational
  901   5     Thuraya RMSS Network    Operational
  901   6     Thuraya Satellite Telecommunications Company    Operational
  901   10    ACeS  ACeS  Operational
  901   11    Inmarsat  Inmarsat  Operational
  901   12    Maritime Communications Partner AS  MCP   Operational
  901   13    Global Networks Switzerland Inc.  GSM.AQ  Operational
  901   14    AeroMobile AS     Operational
  901   15    OnAir Switzerland Sarl    Operational
  901   17    Navitas   Navitas   Operational
  901   18    AT&T Mobility   Cellular @Sea   Operational
  901   21    Seanet Maritime Communications  Seanet  Operational

  Iran
  432   11    Mobile Communications Company of Iran   IR-MCI  Operational
  432   14    KIFZO (Telecommunication Kish Co.)  TKC   Operational
  432   19    Mobile Telecommunications Company of Esfahan  MTCE  Operational
  432   32    Rafsanjan Industrial Complex  Taliya  Operational
  432   35    Irancell Telecommunications Services Company  Irancell  Operational
  432   70    Telephone Communications Company of Iran  TCI   Operational
  432   93      Iraphone  Operational

  Iraq
  418   5     Asia Cell Telecommunications Com.   Asia Cell   Operational
  418   8     Korek Telecom Ltd   Korek   Operational
  418   20    Zain Iraq   Zain  Operational
  418   30    Zain Iraq   Zain  Operational
  418   40    Korek Telecom Ltd   Korek   Operational
  418   45    Mobitel Co. Ltd   Mobiltel  Operational
  418   92    Ommnea Wireless   Omnnea  Operational

  Ireland
  272   1     Vodafone Ireland Plc  Vodafone  Operational
  272   2     O2 Ireland  O2  Operational
  272   3     Meteor Mobile Communications Ltd.   Meteor  Operational
  272   4     Access Telecom Ltd.     Inactive
  272   5     Hutchison 3G Ireland limited  3   Operational
  272   7     Eircom  Eircom  Operational
  272   9     Clever Communications Ltd.    Inactive
  272   11    Liffey Telecom  Tesco Mobile  Operational
  272   13      Lyca Mobile   Operational

  Israel
  425   1     Partner Communications Co. Ltd.   Orange  Operational
  425   2     Cellcom Israel Ltd.   Cellcom   Operational
  425   3     Pelephone Communications Ltd.   Pelephone   Operational
  425   5     Palestine Cellular Communications   Jawwal  Operational
  425   6       Wataniya Palestine  Inactive
  425   7     Hot Mobile  Hot Mobile  Operational
  425   8       Golan Telecom   Operational
  425   303     Pelephone Communications Ltd.   Rami Levy   Operational
  */

  //Italy
  {222,   1,    "Telecom Italia SpA",   "TIM I"},
  {222,   2,    "Elsacom",  "Elsacom I"},
  {222,   7,    "Noverca",  "Noverca I"},
  {222,   8,    "Fastweb", "Fastweb I"},
  {222,   10,     "Vodafone Omnitel N.V.",  "Vodafone I"},
  {222,   30,     "Rete Ferroviaria Italiana",  "RFI I"},
  {222,   77,     "IPSE 2000", ""},
  {222,   88,     "Wind Telecomunicazioni SpA",   "Wind I"},
  {222,   98,     "Blu", ""},
  {222,   99,     "Hutchison 3G",   "3 Italia"},
  /*
  Jamaica
  338   5     JM DIGICEL    Inactive
  338   20    Cable & Wireless Jamaica Ltd.   LIME (formerly known as Cable & Wireless)   Operational
  338   50    Digicel (Jamaica) Limited   Digicel   Operational
  338   70    Oceanic Digital Jamaica Limited   Claro   Operational
  338   180     Cable and Wireless Jamaica Limited  LIME (formerly known as Cable & Wireless)   Operational

  Japan
  440   0     Emobile Ltd   eMobile   Operational
  440   1     NTT DoCoMo  DoCoMo  Operational
  440   2     NTT DoCoMo Kansai   DoCoMo  Operational
  440   3     NTT DoCoMo Hokuriku   DoCoMo  Operational
  440   4     SoftBank Mobile Corp  Softbank  Operational
  440   6     Softbank Mobile Corp  Softbank  Operational
  440   7     KDDI Corporation  KDDI  Operational
  440   8     KDDI Corporation  KDDI  Operational
  440   9     NTT DoCoMo Kansai   DoCoMo  Operational
  440   10    NTT DoCoMo  DoCoMo  Operational
  440   11    NTT DoCoMo Tokai  DoCoMo  Operational
  440   12    NTT DoCoMo  DoCoMo  Operational
  440   13    NTT DoCoMo  DoCoMo  Operational
  440   14    NTT DoCoMo Thoku  DoCoMo  Operational
  440   15    NTT DoCoMo  DoCoMo  Operational
  440   16    NTT DoCoMo  DoCoMo  Operational
  440   17    NTT DoCoMo  DoCoMo  Operational
  440   18    NTT DoCoMo Tokai  DoCoMo  Operational
  440   19    NTT DoCoMo Hokkaido   DoCoMo  Operational
  440   20    SoftBank Mobile Corp  SoftBank  Operational
  440   21    NTT DoCoMo  DoCoMo  Operational
  440   22    NTT DoCoMo Kansai   DoCoMo  Operational
  440   23    NTT DoCoMo Tokai  DoCoMo  Operational
  440   24    NTT DoCoMo Chugoku  DoCoMo  Operational
  440   25    NTT DoCoMo Hokkaido   DoCoMo  Operational
  440   26    NTT DoCoMo Kyushu   DoCoMo  Operational
  440   27    NTT DoCoMo Tohoku   DoCoMo  Operational
  440   28    NTT DoCoMo Shikoku  DoCoMo  Operational
  440   29    NTT DoCoMo  DoCoMo  Operational
  440   30    NTT DoCoMo  DoCoMo  Operational
  440   31    NTT DoCoMo Kansai   DoCoMo  Operational
  440   32    NTT DoCoMo  DoCoMo  Operational
  440   33    NTT DoCoMo Tokai  DoCoMo  Operational
  440   34    NTT DoCoMo Kyushu   DoCoMo  Operational
  440   35    NTT DoCoMo Kansai   DoCoMo  Operational
  440   36    NTT DoCoMo  DoCoMo  Operational
  440   37    NTT DoCoMo  DoCoMo  Operational
  440   38    NTT DoCoMo  DoCoMo  Operational
  440   39    NTT DoCoMo  DoCoMo  Operational
  440   40    Softbank Mobile Corp  Softbank  Operational
  440   41    Softbank Mobile Corp  Softbank  Operational
  440   42    Softbank Mobile Corp  Softbank  Operational
  440   43    Softbank Mobile Corp  Softbank  Operational
  440   44    Softbank Mobile Corp  Softbank  Operational
  440   45    Softbank Mobile Corp  Softbank  Operational
  440   46    Softbank Mobile Corp  Softbank  Operational
  440   47    Softbank Mobile Corp  Softbank  Operational
  440   48    Softbank Mobile Corp  Softbank  Operational
  440   49    KDDI Corporation  KDDI  Operational
  440   50    KDDI Corporation  KDDI  Operational
  440   51    KDDI Corporation  KDDI  Operational
  440   52    KDDI Corporation  KDDI  Operational
  440   53    KDDI Corporation  KDDI  Operational
  440   54    KDDI Corporation  KDDI  Operational
  440   55    KDDI Corporation  KDDI  Operational
  440   56    KDDI Corporation  KDDI  Operational
  440   58    NTT DoCoMo Kansai   DoCoMo  Operational
  440   60    NTT DoCoMo Kansai   DoCoMo  Operational
  440   61    NTT DoCoMo Chugoku  DoCoMo  Operational
  440   62    NTT DoCoMo Kyushu   DoCoMo  Operational
  440   63    NTT DoCoMo  DoCoMo  Operational
  440   64    Softbank Mobile Corp  Softbank  Operational
  440   65    NTT DoCoMo Shikoku  DoCoMo  Operational
  440   66    NTT DoCoMo  DoCoMo  Operational
  440   67    NTT DoCoMo Tohoku   DoCoMo  Operational
  440   68    NTT DoCoMo Kyushu   DoCoMo  Operational
  440   69    NTT DoCoMo  DoCoMo  Operational
  440   70    KDDI Corporation  Au  Operational
  440   71    KDDI Corporation  KDDI  Operational
  440   72    KDDI Corporation  KDDI  Operational
  440   73    KDDI Corporation  KDDI  Operational
  440   74    KDDI Corporation  KDDI  Operational
  440   75    KDDI Corporation  KDDI  Operational
  440   76    KDDI Corporation  KDDI  Operational
  440   77    KDDI Corporation  KDDI  Operational
  440   78    Okinawa Cellular Telephone  Okinawa   Operational
  440   79    KDDI Corporation  KDDI  Operational
  440   80    TU-KA Cellular Tokyo  TU-KA   Inactive
  440   81    TU-KA Cellular Tokyo  TU-KA   Inactive
  440   82    TU-KA Phone Kansai  TU-KA   Inactive
  440   83    TU-KA Cellular Tokai  TU-KA   Inactive
  440   85    TU-KA Cellular Tokai  TU-KA   Inactive
  440   86    TU-KA Cellular Tokyo  TU-KA   Inactive
  440   87    NTT DoCoMo Chugoku  DoCoMo  Operational
  440   88    KDDI Corporation  KDDI  Operational
  440   89    KDDI Corporation  KDDI  Operational
  440   90    Softbank Mobile Corp  Softbank  Operational
  440   91    Softbank Mobile Corp  Softbank  Operational
  440   92    SoftBank Mobile Corp  Softbank  Operational
  440   93    Softbank Mobile Corp  Softbank  Operational
  440   94    Softbank Mobile Corp  Softbank  Operational
  440   95    Softbank Mobile Corp  Softbank  Operational
  440   96    Softbank Mobile Corp  Softbank  Operational
  440   97    Softbank Mobile Corp  Softbank  Operational
  440   98    Softbank Mobile Corp  Softbank  Operational
  440   99    NTT DoCoMo  DoCoMo  Operational

  Jordan
  416   1     Jordan Mobile Telephone Services  zain JO   Operational
  416   2     Xpress  XPress Telecom  Operational
  416   3     Umniah  Umniah  Operational
  416   77    Petra Jordanian Mobile Telecommunications Company (MobileCom)   Orange  Operational

  Kazakhstan
  401   1     Kar-Tel llc   Beeline   Operational
  401   2     GSM Kazakhstan Ltd  Kcell   Operational
  401   7     Dalacom (CDMA)  Dalacom   Operational
  401   8     Kazakhtelecom   Kazakhtelecom   Operational
  401   77    Mobile Telecom Service LLP  Mobile Telecom Service  Operational

  Kenya
  639   2     Safaricom Ltd.  Safaricom   Operational
  639   3     Celtel Kenya Limited  Zain  Operational
  639   5     Telkom Kenya  yu  Operational
  639   7     Econet Wireless Kenya   Orange Kenya  Operational

  Kiribati
  545   9     Telecom Services Kiribati Ltd   Kiribati Frigate  Operational

  Korea (North)
  467   193     Korea Posts and Telecommunications Corporation  SUN NET   Operational

  Korea (South)
  450   0     KT  Dacom   Inactive
  450   2     KT  KT  Operational
  450   3     Shinsegi Telecom Inc.   Digital 017   Inactive
  450   4     KT  KT  Operational
  450   5     SK Telecom  SKT   Operational
  450   6     LGT Telcom  LGT   Operational
  450   8     KTF SHOW  KTF Show  Operational

  Kuwait
  419   2     Mobile Telecommunications Company   Zain  Operational
  419   3     Wataniya Telecom  Wataniya  Operational
  419   4     Kuwait Telecommunication Company  Viva  Operational

  Kyrqyzstan
  437   1     Bitel GSM     Operational
  437   3     Aktel Ltd   Fonex   Operational
  437   5     MEGACOM     Operational
  437   9     NurTelecom LLC  O!  Operational

  Laos
  457   1     Lao Shinawatra Telecom  LaoTel  Operational
  457   2     Enterprise of Telecommunications Lao  ETL   Operational
  457   3     Star Telecom Co., Ltd   Unitel  Operational
  457   8     Millicom Lao Co Ltd   Tigo  Operational

  Latvia
  247   1     Latvian Mobile Phone (LMT)  LMT   Operational
  247   2     Tele2   Tele2   Operational
  247   3     Telekom Baltija (CDMA)  Triatel   Operational
  247   5     Bite Latvija  Bite  Operational
  247   6     Rigatta     Inactive
  247   7     Master Telecom (MTS)    Operational
  247   8     IZZI    Operational
  247   9     Camel Mobile  Camel Mobile  Operational

  Lebanon
  415   1     MIC 1   Alfa  Operational
  415   3     MIC 2   mtc touch   Operational
  415   5     Ogero Telecom   Ogero Mobile  Operational

  Lesotho
  651   1     Vodacom Lesotho (pty) Ltd.  Vodacom   Operational
  651   2     Econet Ezin-cel     Operational

  Liberia
  618   1     Lonestar Cell     Operational
  618   2     Libercell     Inactive
  618   3     Cellcom     Operational
  618   4     Comium    Operational
  618   7     Cellcom   Cellcom   Operational
  618   20    LIB Telco     Inactive

  Libya
  606   0     Libyana Mobile Phone    Operational
  606   1     Madar     Operational

  Liechtenstein
  295   1     Swisscom AG   Swisscom  Operational
  295   2     Orange AG   Orange  Operational
  295   4     Cubic Telecom AG  Cubic Telecom   Operational
  295   5     Mobilkom AG (FL1)   FL1   Operational
  295   77    Alpcom AG   Alpmobil  Operational

  Lithuania
  246   1     Omnitel     Operational
  246   2     Bite GSM    Operational
  246   3     Tele2     Operational

  Luxembourg
  270   1     LuxGSM  LuxGSM  Operational
  270   77    Tango   Tango   Operational
  270   99    Orange S.A.   Orange  Operational

  Macao China
  455   0     SmarTone    Operational
  455   1     C.T.M Telemovel+  CTM   Operational
  455   2     China Telecom     Operational
  455   3     Hutchison Telecom   3   Operational
  455   4     C.T.M Telemovel+  CTM   Operational
  455   5     Hutchison Telecom   3   Operational

  Macedonia
  294   1     T-Mobile    Operational
  294   2     Cosmofon    Operational
  294   3     VIP Operator    Operational

  Madagascar
  646   1     Celtel  Zain  Operational
  646   2     Orange Madagascar S.A   Orange  Operational
  646   3     Madamobil   Madamobil   Operational
  646   4     Telma Mobile S.A  Telma   Operational

  Malawi
  650   1     Telekom Network Ltd.  TNM   Operational
  650   10    Bharti Airtel Limited   Airtel  Operational

  Malaysia
  502   0     Art900    Inactive
  502   1     Telekom Malaysia Bhd  TM CDMA   Operational
  502   11    MTX Utara     Inactive
  502   12    Maxis Communications Berhad   Maxis   Operational
  502   13    TM Touch  Celcom  Operational
  502   16    DIGI  DiGi  Operational
  502   17    Maxis   Maxis   Operational
  502   18    U Mobile Sdn Bhd  U Mobile  Operational
  502   19    CelCom  Celcom  Operational

  Maldives
  472   1     Dhivehi Raajjeyge Gulhun  Dhiraagu  Operational
  472   2     Wataniya Telecom Maldives   Wataniya  Operational

  Mali
  610   1     Malitel   Malitel   Operational
  610   2     Orange Mali SA  Orange  Operational

  Malta
  278   1     Vodafone Malta  Vodafone  Operational
  278   21    Mobisle Communications Limited  GO  Operational
  278   77    Melita Plc  Melita  Operational

  Martinique & Guadeloupe
  340   1     Orange Caraïbe Mobiles   Orange  Operational
  340   2     Outremer Telecom  Only  Operational
  340   3     Saint Martin et Saint Barthelemy Telcell Sarl   Telcell   Operational
  340   8     Dauphin Telecom   Dauphin   Operational
  340   20    DIGICEL Antilles Française Guyane  Digicel   Operational
  340   993     Orange Martinique     Operational

  Mauritania
  609   1     Mattel S.A.   Mattel  Operational
  609   2     Chinguitel S.A.     Operational
  609   10    Mauritel Mobiles  Mauritel  Operational

  Mauritius
  617   1     Cellplus Mobile Communications Ltd.   Orange  Operational
  617   2     Mahanagar Telephone (Mauritius) Ltd.  MTML  Operational
  617   10    Emtel Ltd   Emtel   Operational

  Mexico
  334   1     Nextel México  Nextel  Operational
  334   2     América Móvil   Telcel  Operational
  334   3     Telefonica Moviles (Movistar)   Movistar  Operational
  334   4     Iusacell / Unefon   Iusacell / Unefon   Operational
  334   5       Iusacell / Unefon   Operational
  334   20    América Móvil   Telcel  Inactive
  334   50    Iusacell / Unefon   Iusacell / Unefon   Operational

  Micronesia
  550   1     FSM Telecom     Operational

  Moldova
  259   1     Orange Moldova GSM  Orange  Operational
  259   2     Moldcell GSM  Moldcell  Operational
  259   3     Moldtelecom   IDC   Operational
  259   4     Eventis Mobile GSM  Evntis  Inactive
  259   5     Moldtelecom   Unite   Operational
  */

  //Monaco
  {212,   0,      "Media Telecom", ""},
  {212,   1,    "Monaco Telecom", "Office des Telephones Monaco"},
  {212,   2,    "Morocco Wana", "Wana Monaco"},
  /*
  Mongolia
  428   88    Unitel  Unitel  Operational
  428   91    Skytel LLC  Skytel  Operational
  428   98    G-Mobile LLC  G.Mobile  Operational
  428   99    Mobicom   MobiCom   Operational

  Montenegro
  297   1     Telenor Montenegro  Telenor   Operational
  297   2     T-Mobile Montenegro LLC   T-Mobile  Operational
  297   3     MTEL CG   m:tel CG  Operational
  297   4     T-Mobile Montenegro LLC   T-Mobile  Operational

  Montserrat
  354   860     Cable & Wireless West Indies (Montserrat)     Operational

  Morocco
  604   0     Medi Telecom  Méditel  Operational
  604   1     Ittissalat Al Maghrib (Maroc Telecom)   IAM   Operational
  604   2     WANA - Groupe ONA   INWI  Operational
  604   5     WANA - Groupe ON  INWI  Operational

  Mozambique
  643   1     Mocambique Celular S.A.R.L  mCel  Operational
  643   3     Movitel   Movitel   Operational
  643   4     Vodacom Mozambique, S.A.R.L.  Vodacom   Operational

  Myanmar
  414   1     Myanmar Post and Telecommunication  MPT   Operational
  414   5     Ooredoo Myanmar   Ooredoo   Operational
  414   6     Telenor Myanmar   Telenor   Operational

  Namibia
  649   1     Mobile Telecommunications Ltd.  MTC   Operational
  649   2     Switch  Switch  Operational
  649   3     Orascom Telecom Holding   Leo   Operational

  Nepal
  429   1     Nepal Telecommunications  Namaste / Nt Mobile   Operational
  429   2     Spice Nepal Private Ltd.  Ncell   Operational
  429   3     Nepal Telecom   Sky/C-Phone   Operational
  429   4     Smart Telecom Pvt. Ltd  SmartCell   Operational
  */

  //Netherlands
  {204,   1,    "VastMobiel B.V.",  "Scarlet Telecom B.V"},
  {204,   2,    "Tele2",  "Tele2 NL"},
  {204,   3,    "Voiceworks B.V",   "Voiceworks B.V"},
  {204,   4,    "Vodafone",   "Vodafone NL"},
  {204,   5,    "Elephant Talk Communications", "Elephant NL"},
  {204,   6,    "Mundio Mobile Ltd",  "Mundio Mobile NL"},
  {204,   7,    "Teleena Holding B.V.",   "Teleena NL"},
  {204,   8,    "KPN B.V",  "KPN NL"},
  {204,   9,    "Lyca mobile Netherlands Ltd.",   "Lyca mobile NL"},
  {204,   10,     "KPN B.V",  "KPN NL"},
  {204,   12,     "Telfort B.V",  "Telfort NL"},
  {204,   13,     "Unica Installatietechniek B.V.",""},
  {204,   14,     "6GMobile B.V",   "6GMobile NL"},
  {204,   15,     "Ziggo B.V",  "Ziggo B.V NL"},
  {204,   16,     "T-Mobile",   "T-mobile NL"},
  {204,   17,     "Intercity Mobile Communications B.V",  "Intercity NL"},
  {204,   18,     "UPC Netherlands B.V",  "UPC NL"},
  {204,   19,     "Mixe Communication Solutions B.V",""},
  {204,   20,     "T-mobile",   "T-mobile NL"},
  {204,   21,     "ProRail B.V",""},
  {204,   22,     "Ministerie van Defensie",""},
  {204,   23,     "ASPIDER Solutions B.V",""},
  {204,   24,     "Private Mobility Netherlands B.V",   ""},
  {204,   25,     "CapX B.V.", ""},
  {204,   26,     "SpeakUp B.V",""},
  {204,   27,     "Brezz Nederland B.V", ""},
  {204,   28,     "Lancelot B.V", ""},
  {204,   67,     "RadioAccess B.V.",""},
  {204,   68,     "Unify Group Holding B.V",""},
  {204,   69,     "KPN B.V",  "KPN NL"},
  /*
  Netherlands Antilles
  362   51    TelCell     Operational
  362   69    Digicel     Operational
  362   76    Antiliano Por N.V.  Antiliano Por N.V.  Operational
  362   91    UTS     Operational
  362   94    Bayos   Bayos   Operational
  362   95    MIO   MIO   Operational
  362   630     Cingular Wireless     Operational
  362   951     UTS Wireless Curacao    Operational

  New Caledonia
  546   1     OPT Mobilis   Mobilis   Operational

  New Zealand
  530   0     Telecom New Zealand   Telecom   Operational
  530   1     Vodafone New Zealand  Vodafone  Operational
  530   2     Telecom New Zealand   Telecom   Inactive
  530   3     Woosh Wireless New Zealand  Woosh   Operational
  530   4     TelstraClear New Zealand  TelstraClear  Operational
  530   5     Telecom New Zealand   XT Mobile (Telecom)   Operational
  530   6     Skinny  Skinny  Operational
  530   24    2degrees  2degrees  Operational
  530   28    Econet Wireless New Zealand GSM Mobile Network    Inactive
  530   55    Cable & Wireless Guernsey Ltd   Telecom   Operational

  Nicaraqua
  710   21    Empresa Nicaragüense de Telecomunicaciones, S.A. (ENITEL)  Claro   Operational
  710   30    Telefónica Móviles de Nicaragua S.A.  movistar  Operational
  710   73    Servicios de Comunicaciones, S.A. (SERCOM)  SERCOM  Operational
  710   730     SERCOM S.A. (Nicaragua)     Inactive

  Niger
  614   1     Sahel.Com   SahelCom  Operational
  614   2     Celtel  Zain  Operational
  614   3     Telecel Niger SA  Telecel   Operational
  614   4     Orange  Orange  Operational

  Nigeria
  621   20    Bharti Airtel Ltd   Airtel  Operational
  621   25    Vidafone Communications Ltd.  Visafone  Operational
  621   30    MTN Nigeria Communications  MTN   Operational
  621   40    Nigerian Mobile Telecommunications Ltd.   M-Tel   Operational
  621   50    Globacom Ltd  Glo   Operational
  621   60    Etisalat Ltd.   Etisalat  Operational

  Norway
  242   1     Telenor Norway AS   Telenor   Operational
  242   2     TeliaSonera Norway AS   Netcom  Operational
  242   3     Teletopia Gruppen AS  MTU   Inactive
  242   4     Tele2 Norway AS   Tele2   Operational
  242   5     Network Norway AS   Network Norway  Operational
  242   6     ICE Norge AS  ICE   Inactive
  242   7     Ventelo Norway AS   Ventelo   Operational
  242   8     TDC Mobil AS  TDC   Operational
  242   9     Com4 AS   Com4  Inactive
  242   11    Systemnet AS    Inactive
  242   12    Telenor Norway AS   Telenor   Inactive
  242   20      Jernbaneverket  Inactive
  242   21      Jernbaneverket  Inactive
  242   22    Network Norway AS   Network Norway  Inactive
  242   23    Lyca Mobile Ltd     Operational

  Oman
  422   2     Oman Mobile Telecommunications Company  Oman Mobile   Operational
  422   3     Oman Qatari Telecommunications Company (Nawras)   Nawras  Operational
  422   4     Oman Telecommunications Company (Omantel)     Inactive

  Pakistan
  410   1     Mobilink-PMCL   Mobilink  Operational
  410   3     Pakistan Telecommunication Mobile Ltd   Ufone   Operational
  410   4     China Mobile  Zong  Operational
  410   6     Telenor Pakistan  Telenor   Operational
  410   7     WaridTel  Warid   Operational
  410   8     Instaphone (AMPS/CDMA)    Inactive

  Palau
  552   1     Palau National Communications Corp. (a.k.a. PNCC)   PNCC  Operational
  552   80    Palau Mobile Corporation  Palau Mobile  Operational

  Panama
  714   1     Cable & Wireless Panama S.A.  Cable & Wireless  Operational
  714   2     Telefonica Moviles Panama S.A   movistar  Operational
  714   3     América Móvil   Claro   Operational
  714   4     Group   Digicel   Operational
  714   20    Movistar    Inactive

  Papua New Guinea
  537   1     Pacific Mobile Communications   B-Mobile  Operational
  537   2     Greencom    Inactive
  537   3     Digicel PNG   Digicel   Operational

  Paraquay
  744   1     Hola Paraguay S.A   VOX   Operational
  744   2     AMX Paraguay S.A.   Claro   Operational
  744   3     Compañia Privada de Comunicaciones S.A.    Inactive
  744   4     Telefonica Celular Del Paraguay S.A. (Telecel)  Claro   Operational
  744   5     Núcleo S.A   Personal  Operational

  Peru
  716   6     Telefónica Móviles Perú  movistar  Operational
  716   7     Nextel    Inactive
  716   10    América Móvil Perú   Claro   Operational
  716   17    NII Holdings  NEXTEL  Operational

  Philippines
  515   1     Globe Telecom via Innove Communications   Islacom   Inactive
  515   2     Globe Telecom   Globe   Operational
  515   3     PLDT via Smart Communications   Smart   Operational
  515   5     Digital Telecommunications Philippines  Sun   Operational
  515   11    PLDT via ACeS Philippines     Inactive
  515   18    PLDT via Smarts Connectivity Unlimited Resources Enterprise   Cure  Operational
  515   88    Nextel  Nextel  Operational
  */

  //Poland
  {260,   1,    "Plus GSM (Polkomtel S.A.)",  "Plus (Polkomtel)"},//  Operational
  {260,   2,    "ERA GSM (Polska Telefonia Cyfrowa Sp. Z.o.o.)",  "T-Mobile"},//  Operational
  {260,   3,    "Orange",   "Orange"},//  Operational
  {260,   4,    "Netia S.A",  "Tele 2 (Netia)"},//  Inactive
  {260,   5,    "Polska Telefonia Komórkowa", "Centertel Sp. z o.o."},//     Inactive
  {260,   6,    "Play (P4) 	Play (P4)"},//  Operational
  {260,   7,    "Netia S.A 	Netia (Using P4 Nw)"},//  Operational
  {260,   8,    "E-Telko Sp. z o.o."},//    Inactive
  {260,   9,    "Telekomunikacja Kolejowa (GSM-R)"},//    Inactive
  {260,   10,     "Sferia S.A.",  "Sferia (Using T-mobile)"},//   Operational
  {260,   12 ,    "Cyfrowy Polsat S.A.",  "Cyfrowy Polsat"},//  Operational
  {260,   14,     "Sferia S.A",   "Sferia (Using T-mobile)"},//   Operational
  {260,   15,     "CenterNet S.A.",   "CenterNet (UMTS Data only)"},//  Operational
  {260,   16,     "Mobyland Sp. z o.o.",  "Mobyland (UMTS)"},//   Operational
  {260,   17,     "Aero 2 Sp. z o.o.",  "Aero2 (UMTS)"},//  Operational
  /*
  Portugal
  268   1     Vodafone Portugal   Vodafone  Operational
  268   3     Sonaecom – Serviços de Comunicações, S.A.  Optimus   Operational
  268   5     Oniway - Inforcomunicaçôes, S.A.    Inactive
  268   6     Telecomunicações Móveis Nacionais  TMN   Operational
  268   21    Zapp Portugal   Zapp  Operational

  Puerto Rico (US)
  330   10    Cingular Wireless     Inactive
  330   11    Puerto Rico Telephone Company   Claro   Operational

  Quatar
  427   1     Qtel  Qtel  Operational
  427   2     Vodaphone Quatar  Vodafone  Operational

  Republic of Congo
  629   1     Celtel Congo  Zain  Operational
  629   7     MTN CONGO S.A   Libertis Telecom  Operational
  629   10    Libertis Telecom    Operational

  Reunion
  647   0     Orange    Operational
  647   2     Outremer Telecom    Operational
  647   10    SFR Reunion     Operational
  647   995     SRR Mayotte   SRR Mayotte   Operational
  647   997     Orange Mayotte  Orange  Operational

  Romania
  226   1     Vodafone Romania SA     Operational
  226   2     Romtelecom    Operational
  226   3     Cosmote (Zapp)    Operational
  226   4     Cosmote (Zapp)    Operational
  226   5     Digi mobil    Operational
  226   6     Cosmote     Operational
  226   10    Orange Romania    Operational

  Russia
  250   1     Mobile TeleSystems  MTS   Operational
  250   2     MegaFon OJSC  MegaFon   Operational
  250   3     Nizhegorodskaya Cellular Communications   NCC   Operational
  250   4     Sibchallenge    Inactive
  250   5     Yeniseytelecom  ETK   Operational
  250   6     CJSC Saratov System of Cellular Communications  Skylink   Operational
  250   7     Zao SMARTS  SMARTS  Operational
  250   9     Khabarovsky Cellular Phone  Skylink   Operational
  250   10    Don Telecom   DTC   Inactive
  250   11    Orensot     Inactive
  250   12    Baykal Westcom / Akos   Baykal  Operational
  250   13    Kuban GSM   KUGSM   Inactive
  250   15    SMARTS Ufa, SMARTS Uljanovsk  SMARTS  Operational
  250   16    New Telephone Company   NTC   Operational
  250   17    JSC Uralsvyazinform   Utel  Operational
  250   19    Indigo    Inactive
  250   20    Tele2   Tele2   Operational
  250   23    Mobicom - Novisibirsk   Mobicom   Inactive
  250   28    Beeline   Beeline   Inactive
  250   32    K-Telecom   Win Mobile  Operational
  250   35    MOTIV   MOTIV   Operational
  250   37    ZAO Kodotel   ZAO Kodotel   Operational
  250   38    Tambov GSM  Tambov GSM  Operational
  250   39    Uralsvyazinform   Utel  Operational
  250   44    Stuvtelesot     Inactive
  250   50    Mobile TeleSystems  MTS   Operational
  250   92    MTS - Primtelefon     Inactive
  250   93    Telecom XXI     Inactive
  250   99    OJSC VimpelCom  Beeline   Operational

  Rwanda
  635   10    MTN Rwandacell SARL   MTN   Operational
  635   11      Rwandatel (CDMA)  Inactive
  635   12      Rwandatel (GSM)   Operational
  635   13    TIGO RWANDA S.A   Tigo  Operational

  Saint Kitts and Nevis
  356   50    Digicel   Digicel   Operational
  356   70    UTS   Chippie   Operational
  356   110     Cable & Wireless  LIME  Operational

  Saint Lucia
  358   30    Cingular Wireless     Inactive
  358   50    Digicel (St Lucia) Limited  Digicel   Operational
  358   110     Cable & Wireless  Lime (Cable & Wireless)   Operational

  Saint Vincent and the Grenadines
  360   10    Cingular Wireless     Inactive
  360   70    Digicel (St. Vincent and Grenadines) Limited  Digicel   Operational
  360   100     Cingular Wireless   Cingular Wireless   Operational
  360   110     Cable & Wireless  Lime (Cable & Wireless)   Operational

  Samoa
  549   1     Digicel Pacific Ltd.  Digicel   Operational
  549   27    SamoaTel Ltd  SamoaTel  Operational

  Sao Tome and Principe
  626   1     Companhia Santomese de Telecomunicaçôes   CSTmovel  Operational

  Saudi Arabia
  420   1     Saudi Telecom Company   Al Jawal  Operational
  420   3     Etihad Etisalat Company   Mobily  Operational
  420   4     Zain SA   Zain SA   Operational
  420   7     EAE     Inactive

  Back to top
  Senegal
  MCC   MNC   Network   Operator or brand name  Status
  608   1     Sonatel   Orange  Operational
  608   2     Millicom  Tigo  Operational
  608   3     Sudatel   Expresso  Operational

  Back to top
  Serbia
  MCC   MNC   Network   Operator or brand name  Status
  220   1     Telenor Serbia  Telenor   Operational
  220   2     Telenor Montenegro  Telenor   Operational
  220   3     Telekom Srbija a.d.   mt:s  Operational
  220   4     Monet     Inactive
  220   5     Vip mobile d.o.o.   VIP   Operational

  Seychelles
  633   1     Cable & Wireless (Seychelles) Ltd.  Cable & Wireless  Operational
  633   2     Mediatech International Ltd.  Mediatech International   Operational
  633   10    Telecom (Seychelles) Ltd.   Airtel  Operational

  Sierra Leone
  619   1     Bharti Airtel Ltd   Airtel  Operational
  619   2     Millicom (SL) Ltd   Tigo  Operational
  619   3     Lintel Sierra Leone Ltd   Africell  Operational
  619   4     Comium (Sierra Leone) Ltd.  Comium  Operational
  619   5     Lintel (Sierra Leone) Ltd.  Africell  Operational
  619   25    Mobitel   Mobitel   Operational
  619   40    Datatel (SL) Ltd GSM    Inactive
  619   50    Dtatel (SL) Ltd CDMA    Inactive
  619   999     Sierratel   LeoneCel  Inactive

  Singapore
  525   1     Singapore Telecom   SingTel   Operational
  525   2     Singapore Telecom   SingTel-G18   Operational
  525   3     MobileOne Asia  M1  Operational
  525   5     StarHub Mobile  StarHub   Operational
  525   12    Digital Trunked Radio Network     Operational

  Slovakia
  231   1     Orange Slovensko  Orange  Operational
  231   2     T-Mobile Slovensko  T-Mobile  Operational
  231   3     Unient Communications     Inactive
  231   4     T-Mobile Slovensko  T-Mobile  Operational
  231   5     Orange Slovensko  Orange  Inactive
  231   6     Telefónica O2  O2  Operational
  231   15    Orange Slovensko    Inactive
  231   99    Železnice Slovenskej Republiky   ŽSR  Inactive

  Slovenia
  293   40    SI Mobil D.D  Si.mobil  Operational
  293   41    Ipkonet   Mobitel   Operational
  293   64    T-2 d.o.o.  T-2   Operational
  293   70    Tusmobil d.o.o.   Tušmobil   Operational

  Solomon Islands
  540   1     Solomon Telekom Co Ltd  BREEZE  Operational
  540   10    Breeze    Inactive

  Somalia
  637   1     Telesom   Telesom   Operational
  637   4     Somafone FZLLC  Somafone  Operational
  637   10    NationLink Telecom  Nationlink  Operational
  637   19    Hormuud Telecom     Inactive
  637   25    Hormuud Telecom Somalia Inc   Hormuud   Operational
  637   30    Golis Telecom Somalia   Golis   Operational
  637   62    Telecom Mobile    Inactive
  637   65    Telecom Mobile    Inactive
  637   82    Telcom Somalia  Telcom  Operational

  South Africa
  655   1     Vodacom (Pty) Ltd.  Vodacom   Operational
  655   2     Telkom  Telkom Mobile / 8.ta  Operational
  655   4     Sasol (PTY) LTD   Sasol (PTY) LTD   Operational
  655   6     Sentech (Pty) Ltd.  Sentech   Operational
  655   7     Cell C (Pty) Ltd.   Cell C & Virgin   Operational
  655   10    Mobile Telephone Networks   MTN   Operational
  655   11    SAPS Gauteng    Operational
  655   13    Neotel  Neotel  Operational
  655   19    Wireless Business Solutions   iBurst  Operational
  655   21    Cape Town Metropolitan Council    Operational
  655   25    Wirels Connect  Wirels Connect  Operational
  655   30    Bokamoso Consortium     Operational
  655   31    Karabo Telecoms (Pty) Ltd.    Operational
  655   32    Ilizwi Telecommunications     Operational
  655   33    Thinta Thinta Telecommunications    Operational
  655   34    Bokone Telecoms     Inactive
  655   35    Kingdom Communications    Inactive
  655   36    Amatole Telecommunication Services    Inactive
  655   41    South African Police Service  South African Police Service  Inactive

  South Sudan
  659   1     Vivacell  Now   Operational

  Spain
  214   1     Vodafone Spain  Vodafone  Operational
  214   3     France Telecom España SA   Orange  Operational
  214   4     Xfera Moviles SA  Yoigo   Operational
  214   5     Telefónica Móviles España  TME   Operational
  214   6     Vodafone Spain  Vodafone  Operational
  214   7     Telefónica Móviles España  movistar  Operational
  214   8     Euskaltel   Euskaltel   Operational
  214   9     France Telecom España SA   Orange  Operational
  214   15    BT Group España Compañia de Servicios Globales de Telecomunicaciones S.A.U.   BT  Operational
  214   16    Telecable de Asturias S.A.U.  TeleCable   Operational
  214   17    R Cable y Telecomunicaciones Galicia S.A.   Móbil R  Operational
  214   18    Cableuropa S.A.U.   ONO   Operational
  214   19    E-PLUS Moviles Virtuales España S.L.U. (KPN)   Simyo   Operational
  214   20    Fonyou Telecom S.L.   Fonyou  Operational
  214   21    Jazz Telecom S.A.U.   Jazztel   Operational
  214   22    Best Spain Telecom  DigiMobil   Operational
  214   23    Barablu Movil Espana  Barablu   Operational
  214   24    Eroski Móvil España   Eroski  Operational
  214   25    LycaMobile S.L.   LycaMobile  Operational
  214   33    France Telecom España SA   Orange  Operational

  Sri Lanka
  413   1     Sri Lanka Telecom Mobitel   Mobitel   Operational
  413   2     Dialog Telekom  Dialog  Operational
  413   3     Emirates Telecommunication Corporation  Etisalat  Operational
  413   5     Bharti Airtel   Airtel  Operational
  413   8     Hutchison Telecommunications Lanka (Pvt) Limited  Hutch   Operational

  St. Helena, Ascension and Tristan
  658   0       Cable & Wireless Plc  Inactive

  St. Pierre and Miquelon
  308   1     St. Pierre-et-Miquelon Télécom  Ameris  Operational

  Sudan
  634   1     Zain Group - Sudan  Zain SD   Operational
  634   2     MTN Sudan   MTN   Operational
  634   5     Wawat Securities  Vivacell (NOW)  Operational
  634   7     Sudatel Group   Sudani One  Operational

  Suriname
  746   1     Telecommunications Company Suriname   Telesur   Inactive
  746   2     Telecommunications Company Suriname   Telesur   Operational
  746   3     Digicel Group Limited   Digicel   Operational
  746   4     Intelsur N.V. / UTS N.V.  Uniqa   Operational

  Swaziland
  653   10    Swazi MTN     Operational

  Sweden
  240   0     Direct2 Internet  Direct2 Internet  Operational
  240   1     Telia Sonera AB   Telia   Operational
  240   2     H3G Access AB   3   Operational
  240   3     Netett Sverige AB   Netett Sverige AB   Operational
  240   5     Svenska UMTS-Nät   Sweden 3G (Telia/Tele2)   Operational
  240   6     Telenor Sweden AB   Telenor   Operational
  240   7     Tele2 Sweden AB   Tele2   Operational
  240   8     Telenor Sweden AB   Telenor   Operational
  240   9     Djuice Mobile Sweden  Djuice Mobile Sweden  Operational
  240   10    Spring Mobil AB   Spring  Operational
  240   11      Lindholmen Science Park   Operational
  240   12    Lycamobile AB   Lycamobile  Operational
  240   13    Ventelo Sweden AB   Ventelo   Operational
  240   14    TDC Mobil AS  TDC   Operational
  240   15    Wireless Maingate   Wireless Maingate   Inactive
  240   16    42 Telecom AB   42 Telecom AB   Operational
  240   17    Götalandsnätet AB   Götalandsnätet AB   Operational
  240   18    Generic Mobile Systems Sweden AB    Operational
  240   19    Mudio Mobile Sweden   Mudio Mobile  Operational
  240   20    Imez AB   Imez AB   Inactive
  240   22      EuTel   Operational
  240   23      Infobip Ltd   Inactive
  240   25      Digitel Mobile Srl  Operational
  240   26    Beepsend  Beepsend  Inactive
  240   27    MyIndian AB   MyIndian AB   Operational
  240   28      CoolTEL Aps   Inactive
  240   29      Mercury International Carrier Services  Operational
  240   30      NextGen Mobile Ltd  Operational
  240   32      CompaTel Ltd.   Inactive
  240   34    Tigo LTD  Tigo LTD  Operational
  240   36    Interactive digital media GmbH  IDM   Operational
  240   41      Shyam Telecom UK Ltd  Operational
  240   89    ACN Communications Sweden AB  ACN Communications Sweden AB  Operational
  240   500     Unknown   Unknown   Operational
  240   503     Unknown   Unknown   Operational

  Switzerland
  228   1     Swisscom Ltd  Swisscom  Operational
  228   2     Sunrise Communications AG   Sunrise   Operational
  228   3     Orange Communications SA  Orange  Operational
  228   5     Togewanet AG (Comfone)    Operational
  228   6     SBB AG  SBB AG  Operational
  228   7     IN&Phone SA   IN&Phone  Operational
  228   8     Tele2 Telecommunications AG   Tele2   Operational
  228   9     Comfone AG  Comfone   Inactive
  228   12    Sunrise     Inactive
  228   50    3G Mobile AG    Operational
  228   51    Bebbicell AG    Operational
  228   52    Barablu Mobile AG   Barablu   Inactive
  228   53    UPC Cablecom GmbH   UPC   Inactive
  228   54    Lyca Mobile AG  Lyca Mobile   Inactive

  Syrian Arab Republic
  417   1     Syriatel Mobile Telecom   Syriatel  Operational
  417   2     MTN Syria   MTN   Operational

  Taiwan
  466   1     Far EasTone Telecommunications Co Ltd   FarEasTone  Operational
  466   5     Asia Pacific Telecom  APTG  Operational
  466   6     Tuntex Telecom  Tuntex  Operational
  466   11    LDTA/Chungwa Telecom  Chunghwa LDM  Operational
  466   68    ACeS Taiwan Telecommunications Co Ltd   ACeS  Inactive
  466   88    KG Telecom  KG Telecom  Operational
  466   89    VIBO Telecom  VIBO  Operational
  466   92    Chunghwa Telecom LDM  Chungwa   Operational
  466   93    Mobitai Communications  MobiTai   Operational
  466   97    Taiwan Cellular Corporation   Taiwan Mobile   Operational
  466   99    TransAsia Telecoms  TransAsia   Operational

  Tajikistan
  436   1     JV Somoncom   Tcell   Operational
  436   2     Indigo Tajikistan   Tcell   Operational
  436   3     TT Mobile   MLT   Operational
  436   4     Babilon-Mobile  Babilon-M   Operational
  436   5     Vimpelcom   Beeline   Operational
  436   12    Indigo  Tcell   Operational

  Tanzania
  640   1     Tri Telecommunication (T) Ltd.    Inactive
  640   2     MIC Tanzania Ltd  tiGO  Operational
  640   3     Zanzibar Telecom Ltd  Zantel  Operational
  640   4     Vodacom Tanzania Ltd  Vodacom   Operational
  640   5     Bharti Airtel   Airtel  Operational
  640   6     Dovetel Ltd   SasaTel   Inactive
  640   7     Tanzania Telecommunication Company LTD  TTCL Mobile   Inactive
  640   8     Benson Informatics Ltd  Benson Online (BOL)   Inactive
  640   9     ExcellentCom Tanzania Ltd   Hits  Inactive
  640   11    Smile Telecoms Holdings Ltd   SmileCom  Inactive

  Thailand
  520   0     CAT Telecom   My by CAT   Operational
  520   1     Advanced Info Service   AIS   Operational
  520   2     CAT Telecom   CAT   Operational
  520   3     AWN   AIS 3G  Operational
  520   4     Real Future   True Move   Operational
  520   5     DTN   dTac  Operational
  520   10      WCS   Inactive
  520   15    Telephone Organization of Thailand  TOT 3G  Operational
  520   18    Total Access Communications (DTAC)  dTac  Operational
  520   23    Digital Phone (AIS)   AIS   Operational
  520   25    True Corporation  WE PCT  Operational
  520   88    True Corporation  True Move H   Inactive
  520   99    True Corporation  True Move   Operational

  Timor-Leste (East Timor)
  514   2     Timor Telecom     Operational

  Togo
  615   1     Togo Telecom  Togo Cell   Operational
  615   2     Telecel     Inactive
  615   3     Moov Togo   Moov  Operational
  615   5     Telecel     Inactive

  Tonga
  539   1     Tonga Communications Corporation    Operational
  539   43    Shoreline Communication     Operational
  539   88    Digicel   Digicel   Operational

  Trinidad and Tobago
  374   12    BMobile   bMobile   Operational
  374   13    Digicel   Digicel   Operational
  374   140     LaqTel Ltd.     Inactive

  Tunisia
  605   1     Orange Tunisia  Orange  Operational
  605   2     Tunisie Telecom   Tunicell  Operational
  605   3     Orascom Telecom   Tunisiana   Operational

  Turkey
  286   1     Turkcell Iletisim Hizmetleri A.S.   Turkcell  Operational
  286   2     Vodafone  Vodafone  Operational
  286   3     Avea  Avea  Operational
  286   4     Aycell  Aycell  Inactive

  Turkmenistan
  438   1     Barash Communication Technologies (BCTI)  MTS   Operational
  438   2     TM-Cell   TM-Cell   Operational

  Turks and Caicos Islands (UK)
  376   5     Digicel (Turks & Caicos) Limited  Digicel   Operational
  376   350     Cable & Wireless West Indies Ltd (Turks & Caicos)   Lime (Cable & Wireless)   Operational
  376   352     IslandCom Communications Ltd.   Islandcom   Operational

  Uganda
  641   1     Celtel Uganda   Zain  Operational
  641   10    MTN Uganda Ltd.   MTN   Operational
  641   11    Uganda Telecom Ltd.   Uganda Telecom  Operational
  641   14    Orange Uganda   Orange  Operational
  641   22    Warid Telecom Uganda Ltd.   Warid Telecom   Operational

  UK
  234   0     British Telecom   BT  Operational
  234   1     Mudio Mobile Ltd  Vectone MObile  Operational
  234   2     O2 UK Ltd   O2  Operational
  234   3     Jersey Airtel Ltd   Airtel-Vodafone   Operational
  234   4     FMS Solutions Ltd     Inactive
  234   5     COLT Mobile Telecommunications Ltd    Inactive
  234   6     Internet Computer Bureau Ltd    Inactive
  234   7     Cable and Wireless Plc    Operational
  234   8     OnePhone Ltd    Inactive
  234   9     Tismi BV    Inactive
  234   10    Telefónica Europe  O2  Operational
  234   11    Telefónica Europe  O2  Operational
  234   12    Network Rail Infrastructure Ltd   Railtrack Plc (UK)  Inactive
  234   13    Network Rail Infrastructure Ltd   Railtrack Plc (UK)  Inactive
  234   14    Hay Systems Ltd     Inactive
  234   15    Vodafone Ltd  Vodafone  Operational
  234   16    Talk Talk Communications Ltd  Talk Talk   Operational
  234   17    Flextel Ltd     Operational
  234   18    Wire9 Telecom Plc   Cloud9  Operational
  234   19    Teleware Plc  Teleware  Operational
  234   20    Hutchison 3G UK Ltd   3   Operational
  234   22    Routo Telecommunications Ltd  RoutoMessaging  Inactive
  234   24    Stour Marine  Greenfone   Inactive
  234   25      Truphone (UK)   Inactive
  234   30    Everything Everywhere Ltd   T-mobile  Operational
  234   31    Virgin Media (MVNO on EE)   Virgin  Operational
  234   32    Virgin Media (MVNO on EE)   Virgin  Operational
  234   33    Everything Everywhere Ltd   Orange  Operational
  234   34    Everything Everywhere Ltd   Orange  Operational
  234   35    JSC Ingenium Ltd    Inactive
  234   37    Synectiv Ltd    Inactive
  234   50    Jersey Telecom GSM  JT-Wave   Operational
  234   55    Cable and Wireless Plc  Cable and Wireless  Operational
  234   58    Manx Telecom  Manx Telecom  Operational
  234   60    Dobson Telephone Co     Inactive
  234   75    Inquam Telecom Ltd.   Inquam  Operational
  234   76    British Telecom   BT  Operational
  234   78    Airwave     Operational
  234   91    Vodafone United Kingdom   Vodafone  Operational
  234   95    Network Rail Infrastructure Ltd   Railtrack Plc (UK)  Inactive
  234   995     Guernsey Airtel   Guernsey Airtel   Inactive

  Back to top
  Ukraine
  MCC   MNC   Network   Operator or brand name  Status
  255   1     Ukrainian Mobile Communication, UMC   MTS   Operational
  255   2     Ukranian Radio Systems, URS   Beeline   Operational
  255   3     Kyivstar GSM JSC  Kyivstar  Operational
  255   4     Intertelecom  IT  Inactive
  255   5     Golden Telecom  Golden Telecom  Operational
  255   6     Astelit   life:)  Operational
  255   7     Ukrtelecom  Ukrtelecom  Operational
  255   21    CJSC - Telesystems of Ukraine   PEOPLEnet   Operational
  255   23    CDMA Ukraine  ITC   Operational
  255   25    CST Inves   Newtone   Operational
  255   39    Golden Telecom GSM    Inactive
  255   50    Ukrainian Mobile Communications     Inactive
  255   67    Kyivstar    Inactive
  255   68    Wellcom     Inactive

  United Arab Emirates
  424   2     E mirates Telecom Corp  Etisalat  Operational
  424   3     Emirates Integrated Telecommunications Company  du  Operational
  */

  //United States
  {310,   0,    "Mid-Tex Cellular Ltd.",  "Mid-Tex Cellular USA"},
  {311,   0,    "Mid-Tex Cellular Ltd.",  "Mid-Tex Cellular USA"},
  {310,   2,    "Sprint Spectrum",  "Sprint USA"},
  {310,   3,    "Verizon Wireless",   "Verizon USA"},
  {310,   4,    "Verizon Wireless",   "Verizon USA"},
  {310,   5,    "Verizon Wireless",   "Verizon USA"},
  {310,   6,    "Consolidated Telcom",""},
  {310,   7,    "Highland", "Highland USA"},
  {310,   8,    "Corr Wireless Communications",""},
  {310,   9,    "Edge Wireless LLC",""},
  {310,   10,     "Verizon Wirelss",  "Verizon USA"},
  {311,   10,     "Chariton Valley Communications",   "Chariton Valley USA"},
  {310,   11,     "Southern Communications Services Inc.",""},
  {316,   11,     "Southern Communications Services", "Southern USA"},
  {310,   12,     "Verizon Wireless",   "Verizon USA"},
  {310,   13,     "Alltel Wireless",  "Alltel Wireless USA"},
  {310,   14,     "Testing",""},
  {310,   15,     "Southern Communications dba Southern","LINC"},
  /*
  {310,   16,     "Cricket Communications","Cricket USA"},
  310   17    North Sight Communications Inc.     Operational
  310   20    Union Telephone Company     Operational
  311   20    Missouri RSA 5 Partnership    Operational
  310   23    VoiceStream 23    Inactive
  310   24    VoiceStream 24    Inactive
  310   25    VoiceStream 25    Inactive
  */
  {310,   26,     "T-Mobile",   "T-Mobile USA"},
  /*
  311   30    Indigo Wireless   Indigo Wireless   Operational
  310   30    Centennial Communications   Centennial  Operational
  310   31    AERIAL    Inactive
  310   32    IT&E Overseas, Inc.     Inactive
  310   34    Airpeak   Airpeak   Operational
  310   38    USA 3650 AT&T     Inactive
  310   40    Concho Cellular Telephone Co., Inc.   Concho  Operational
  311   40    Commnet Wireless  Commnet Wireless  Operational
  310   46    TMP Corp  Simmetry  Operational
  310   50    ACS Wireless Inc.     Inactive
  311   50    Wikes Cellular  Wikes Cellular  Operational
  311   60    Farmers Cellular Telephone  Farmers Cellular  Operational
  310   60    Consolidated Telcom     Operational
  310   70    Highland Cellular, Inc.     Operational
  */
  {311,   70,     "AT&T",   "AT&T USA"},
  /*
  311   80    Pine Telephone Company  Pine Cellular   Operational
  310   80    Corr Wireless Communications LLC  Corr  Operational
  311   90    Long Lines Wireless LLC   Long Lines Wireless   Operational
  */
  {310,   90,     "AT&T",   "AT&T USA"},
  /*
  311   100     High Plains Wireless  High Plains Wireless  Operational
  310   100     New Mexico RSA 4 East Ltd. Partnership  Plateau Wireless  Operational
  */
  {310,   110,    "Verizon Wireless",   "Verizon USA"},
  {311,   110,    "High Plains Wireless", "High Plains Wireless USA"},
  {310,   120,    "Sprint",   "Sprint USA"},
  /*
  310   130     Carolina West Wireless    Inactive
  311   130     Alltel  Alltel  Operational
  311   140     MBO Wireless  Sprocket  Operational
  310   140     GTA Wireless LLC    Inactive
  */
  {310,   150,    "AT&T Mobility",  "AT&T USA"},
  /*
  311   150     Wilkes Cellular     Operational
  */
  {310,   160,    "T-Mobile",   "T-Mobile USA"},
  {310,   170,    "Cingular Wireless", ""},
  /*
  311   170     Broadpoint Inc  PetroCom  Operational
  310   180     West Central Wireless   West Central  Operational
  310   190     Alaska Wireless Communications LLC  Dutch Harbor  Operational
  310   200     T-Mobile    Inactive
  310   210     T-Mobile    Inactive
  311   210     Farmers Cellular Telephone  Farmers Cellular  Operational
  310   220     T-Mobile    Inactive
  310   230     T-Mobile    Inactive
  310   240     T-Mobile    Inactive
  310   250     T-Mobile    Inactive
  */
  {310,   260,    "T-Mobile", "T-Mobile USA"},
  {311,   260,    "Cellular One",   "Cellular One USA"},
  /*
  311   270     Lamar Country Cellular  Lamar Country Cellular  Operational
  310   270     T-Mobile    Inactive
  310   280     Verizon Wireless  Verizon   Inactive
  310   290     Nep Cellcorp Inc.     Inactive
  311   290     NEP Wireless  NEP Wireless  Operational
  310   300     Smart Call (Truphone)   iSmart Mobile   Operational
  310   310     T-Mobile    Inactive
  310   311     Farmers Wireless    Operational
  310   320     Smith Bagley Inc, dba Cellular One  Cellular One  Operational
  310   330     AN Subsidiary LLC     Inactive
  311   330     Michigan Wireless, LLC  Bug Tussel Wireless   Operational
  310   340     Westlink Communications   Westlink  Operational
  310   350     Mohave Cellular L.P.    Inactive
  310   360     Cellular Network Partnership dba Pioneer Cellular     Inactive
  310   370     Guamcell Cellular and Paging    Inactive
  311   370     General Communication Inc.  GCI Wireless in Alaska  Operational
  310   380     New Cingular Wireless PCS, LLC    Inactive
  */
  {310,   390,    "Verizon Wireless",   "Verizon USA"},
  /*
  310   400     Wave Runner LLC   i CAN_GSM   Operational
  */
  {310,   410,    "AT&T",   "AT&T USA"},
  /*
  310   420     Cincinnati Bell Wireless LLC  Cincinnati Bell   Operational
  310   430     Alaska Digitel LLC    Operational
  310   440     Numerex Corp.     Inactive
  310   450     Viaero Wireless   Viaero  Operational
  310   460     TMP Corp  Simmetry  Operational
  310   470     Omnipoint     Inactive
  */
  {310,   480,    "Verizon Wireless",   "Verizon USA"},
  {310,   490,    "T-Mobile",   "T-Mobile USA"},
  /*
  310   500     Alltel  Alltel  Operational
  310   510     Airtel Wireless LLC   Airtel  Operational
  310   520     VeriSign    Inactive
  310   530     West Virginia Wireless    Operational
  311   530     NewCore Wireless  NewCore Wireless  Operational
  310   540     Oklahoma Western Telephone Company  Oklahoma Western  Operational
  310   550     Wireless Solutions International  AT&T  Inactive
  */
  {310,   560,    "AT&T",   "AT&T USA"},
  /*
  310   570     MTPCS LLC   Cellular One  Operational
  310   580     Inland Cellular Telephone Company     Inactive
  310   590     Alltel  Alltel  Operational
  310   600     New Cell Inc. dba Cellcom     Inactive
  310   610     Elkhart Telephone Co.   Epic Touch  Operational
  310   620     Coleman County Telecommunications Inc. (Trans Texas PCS)  Coleman County Telecom  Operational
  310   630     Choice Wireless   AmeriLink PCS   Operational
  310   640     Airadigm Communications   Airadigm  Operational
  310   650     Jasper Wireless Inc.  Jasper  Operational
  */
  {310,   660,    "MetroPCS",   "MetroPCS USA"},
  /*
  310   670     Northstar   Northstar   Operational
  */
  {310,   680,    "AT&T",   "AT&T USA"},
  /*
  310   690     Conestoga Wireless Company  Conestoga   Operational
  310   700     Cross Valiant Cellular Partnership    Inactive
  310   710     Arctic Slopo Telephone Association Cooperative    Inactive
  310   720     Wireless Solutions International Inc.     Inactive
  310   730     SeaMobile   SeaMobile   Operational
  310   740     Convey Communications Inc.  Convey  Operational
  310   750     East Kentucky Network LLC dba Appalachian Wireless    Inactive
  310   760     Panhandle Telecommunications Systems Inc.   Panhandle   Operational
  310   770     Iowa Wireless Services LLC dba I Wireless   i wireless  Operational
  310   780     Connect Net Inc     Inactive
  310   790     PinPoint Communications Inc.  PinPoint  Operational
  310   800     T-Mobile    Inactive
  310   810     Brazos Cellular Communications Ltd.     Inactive
  310   820     South Canaan Cellular Communications Co. LP     Inactive
  310   830     Caprock Cellular Ltd. Partnership   Caprock   Operational
  310   840     Telecom North America Mobile, Inc.  telna Mobile  Operational
  310   850     Aeris Communications, Inc.  Aeris   Operational
  310   860     TX RSA 15B2, LP dba Five Star Wireless    Inactive
  310   870     Kaplan Telephone Company Inc.   PACE  Operational
  310   880     Advantage Cellular Systems, Inc.  Advantage   Operational
  */
  {310,   890,    "Verizon Wireless",   "Verizon USA"},
  /*
  310   900     Mid-Rivers Communications   Mid-Rivers Wireless   Operational
  */
  {310,   910,    "Verizon Wireless",   "Verizon USA"},
  {0,0,"",""}
  /*
  310   920     Get Mobile    Inactive
  310   930     Copper Valley Wireless    Inactive
  310   940     Poka Lambro Telecommunications Ltd.     Operational
  310   950     Iris Wireless LLC   XIT Wireless  Operational
  310   960     Texas RSA 1 dba XIT Cellular  Plateau Wireless  Operational
  310   970     Globalstar USA  Globalstar  Operational
  310   980     New Cingular Wireless PCS LLC     Inactive
  310   990     E.N.M.R. Telephone Cooperative    Inactive

  Uruguay
  748   0     Compania estatal (ANTEL)  Ancel   Operational
  748   1     Compania estatal (ANTEL)  Ancel   Operational
  748   3     Ancel     Inactive
  748   7     Telefónica Móviles Uruguay  Movistar  Operational
  748   10    AM Wireless Uruguay S.A.  Claro   Operational

  Uzbekistan
  434   1     Buztel    Operational
  434   2     Uzmacom     Operational
  434   4     Unitel LLC  Beeline   Operational
  434   5     Coscom  Ucell   Operational
  434   6     Perfectum Mobile    Operational
  434   7     Uzdunrobita   MTS   Operational

  Vanuatu
  541   1     Telecom Vanatou   Smile   Operational
  541   5     Digicel   Digicel   Operational

  Venezuela
  734   1     Digitel   Digitel   Operational
  734   2     Digitel   Digitel   Operational
  734   3     Digitel   Digitel   Operational
  734   4     Movistar  Movistar  Operational
  734   6     Movilnet  Movilnet  Operational

  Vietnam
  452   1     Vietnam Mobile Telecom (VMS)  MobilFone   Operational
  452   2     Vinaphone   Vinaphone   Operational
  452   3     S-Telecom   S-Fone  Operational
  452   4     Viettel Corporation (Viettel Mobile)  Viettel   Operational
  452   5     Hanoi Telecom   Vietnamobile  Operational
  452   6     EVN Telecom   E-Mobile  Operational
  452   7     GTEL Mobile JSC   Beeline VN  Operational
  452   8     EVN Telecom   EVN Telecom   Operational

  Yemen
  421   1     SabaFon     Operational
  421   2     Spacetel Yemen    Operational
  421   3     Yemen Mobile  Yemen Mobile  Operational
  421   4     Y   HiTS-Unitel   Operational

  Zambia
  645   1     Zain    Operational
  645   2     MTN     Operational
  645   3     Zamtel    Operational

  Zimbabwe
  648   1     Net*One Cellular Ltd  Net One   Operational
  648   3     Telecel Zimbabwe Ltd  Telecel   Operational
  648   4     Econet Wireless   Econet  Operational

  */
};




