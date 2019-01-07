%**************************************************************************
% Calibration of Adjustment Factors for Exponential Effective SINR Mapping
% Methdology for 3GPP Long Term Evolution (LTE)
% Eurecom Institute
% (c) Imran Latif, Communications Mobile, 2012
%**************************************************************************

close all
clear all


userdir= getenv('OPENAIR1_DIR');

addpath(strcat(userdir, '/SIMULATION/LTE_PHY'))

root_path = strcat(userdir, '/SIMULATION/LTE_PHY/data/TM1_ideal_CE');

awgn_path = strcat(userdir, '/SIMULATION/LTE_PHY/BLER_SIMULATIONS/AWGN/Ideal');

transmission_mode = 1;

abstraction_mode = 0;

save_output = 1;

show_plots = 0;

m_path = strcat(root_path, '/mFiles');

if ~exist(m_path,'dir')
    mkdir(fullfile(root_path,'mFiles'));
    error('there are no M Files for the specified folder');
end

EESM_path = strcat(root_path, '/EESM');
if ~exist(EESM_path,'dir')
    mkdir(fullfile(root_path,'EESM'));
end

MIESM_path = strcat(root_path, '/MIESM');
if ~exist(MIESM_path,'dir')
    mkdir(fullfile(root_path,'MIESM'));
end

if (transmission_mode == 5)
    M1_path = strcat(MIESM_path, '/M1');
    if ~exist(M1_path,'dir')
    mkdir(fullfile(MIESM_path,'M1'));
    end

    M2_path = strcat(MIESM_path, '/M2');
    if ~exist(M2_path,'dir')
    mkdir(fullfile(MIESM_path,'M2'));
    end
end

d = dir(fullfile(m_path, '*.mat'));

% if (isempty(d))
%     d = dir(fullfile(m_path, '*.m'));
%     dir_names = {d.name};
%     for d_idx=1:length(dir_names)
%         pathname = fullfile(m_path,dir_names{d_idx})
%        
%     end
%     
%     save(fullfile(m_path,'data_rayleigh.mat'))
%     
% else
    load(fullfile(m_path, d.name));
% end

work_Space_Variables = who('data_all*');

mcs = [];
for s = 1: length(work_Space_Variables)
    c = regexprep(work_Space_Variables(s),'data_all',' ');
    mcs = [mcs str2double(c)];
end

mcs = sort(mcs);


training_abstraction


