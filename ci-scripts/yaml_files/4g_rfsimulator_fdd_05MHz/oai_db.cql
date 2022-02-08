CREATE KEYSPACE IF NOT EXISTS vhss WITH replication = {'class': 'NetworkTopologyStrategy', 'DC1': '1'};

CREATE TABLE IF NOT EXISTS vhss.users_imsi (
    imsi text PRIMARY KEY,
    access_restriction int,
    idmmeidentity int,
    imei text,
    imei_sv text,
    key text,
    lipa_permissions text,
    mme_cap int,
    mmehost text,
    mmeidentity_idmmeidentity int,
    mmerealm text,
    ms_ps_status text,
    msisdn bigint,
    niddvalidity text,
    nir_dest_host text,
    nir_dest_realm text,
    opc text,
    pgw_id int,
    rand text,
    rfsp_index varint,
    sqn bigint,
    subscription_data text,
    ue_reachability varint,
    urrp_mme varint,
    user_identifier text,
    visited_plmnid text);

CREATE TABLE IF NOT EXISTS vhss.msisdn_imsi (
	msisdn bigint PRIMARY KEY,
	imsi text
);

CREATE TABLE IF NOT EXISTS vhss.global_ids (
    table_name text PRIMARY KEY,
    id counter);

CREATE TABLE IF NOT EXISTS vhss.mmeidentity_host (
    mmehost text PRIMARY KEY,
    idmmeidentity int,
    mmerealm text,
    ue_reachability varint,
    mmeisdn text);

CREATE TABLE IF NOT EXISTS vhss.mmeidentity (
    idmmeidentity int PRIMARY KEY,
    mmehost text,
    mmerealm text,
    ue_reachability varint,
    mmeisdn text);

CREATE TABLE IF NOT EXISTS vhss.events (
    scef_id text,
    scef_ref_id bigint,
    extid text,
    monitoring_event_configuration text,
    monitoring_type int,
    msisdn bigint,
    user_identifier text,
    primary key (scef_id, scef_ref_id)
);

CREATE TABLE IF NOT EXISTS vhss.events_msisdn (
    msisdn bigint,
    scef_id text,
    scef_ref_id bigint,
    primary key (msisdn, scef_id, scef_ref_id)
);

CREATE TABLE IF NOT EXISTS vhss.events_extid (
    extid text,
    scef_id text,
    scef_ref_id bigint,
    primary key (extid, scef_id, scef_ref_id)
);

CREATE TABLE IF NOT EXISTS vhss.extid (
    extid text primary key
);

CREATE TABLE IF NOT EXISTS vhss.extid_imsi (
    extid text,
    imsi text,
    primary key (extid, imsi)
);

CREATE TABLE IF NOT EXISTS vhss.extid_imsi_xref (
    imsi text,
    extid text,
    primary key (imsi, extid)
);
