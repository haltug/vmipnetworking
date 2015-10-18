%% Import data from text file.
% Author: Halis Altug, Version: 1.0, Date: 18.10.2015

%% Scenario configuration
N = 96; % number of simulation
sim_time = 450; % simulation duration
seq_thr = 100; % minimum number of elements in data set of a simulation
period_time = 1; % time interval of packet reception
min_pkt_per_period = 10; % minimum number of packets within period
retransmission_thr = 10; % threshold for not tolerable values when ip address is configured

%% Simulation configuration
meas_folder = '/home/halis/vmipnetworking/inet/simulation/frankfurt_urban/output';
main_output_folder = '/home/halis/Halis_MSc_Doc/gfx/results/handover'; 
net_protocol = 'movenet';
communication = '1_to_1';
app_size = 4;
trans_protocol = cell(app_size,1);
trans_protocol{1,1} = 'TCP_CN_to_MA';
trans_protocol{2,1} = 'TCP_MA_to_CN';
trans_protocol{3,1} = 'UDP_CN_to_MA';
trans_protocol{4,1} = 'UDP_MA_to_CN';

for idx_app = 1:app_size
    %% Variables
    close all;
    vecAddressChanges = [];
    vecHandoverDelay = [];
    vecRttMean = [];
    vecRttMin = [];
    vecRttMax = [];

    %% Measurement calculation
    for idx_run = 1:N
        result_folder = [communication '/' trans_protocol{idx_app,1}];
        %% Initialize variables.
        filename1 = [meas_folder '/' result_folder '/' 'rcvd-' num2str(idx_run-1) '.csv'];
        filename2 = [meas_folder '/' result_folder '/' 'ip-' num2str(idx_run-1) '.csv'];
        filename3 = [meas_folder '/' result_folder '/' 'ip_da-' num2str(idx_run-1) '.csv'];
        filename4 = [meas_folder '/' result_folder '/' 'rtt-' num2str(idx_run-1) '.csv'];
        delimiter = ',';
        startRow = 2;

        %% Format string for each line of text:
        formatSpec = '%f%f%[^\n\r]';

        %% Open the text file.
        fileID1 = fopen(filename1,'r');
        fileID2 = fopen(filename2,'r');
        fileID3 = fopen(filename3,'r');
        fileID4 = fopen(filename4,'r');

        %% Read columns of data according to format string.
        % This call is based on the structure of the file used to generate this
        % code. If an error occurs for a different file, try regenerating the code
        % from the Import Tool.
        if(fileID1 > -1)
            dataArray1 = textscan(fileID1, formatSpec, 'Delimiter', delimiter, 'EmptyValue' ,NaN,'HeaderLines' ,startRow-1, 'ReturnOnError', false);
        else
            dataArray1 = cell(2);
        end
        if(fileID2 > -1)
            dataArray2 = textscan(fileID2, formatSpec, 'Delimiter', delimiter, 'EmptyValue' ,NaN,'HeaderLines' ,startRow-1, 'ReturnOnError', false);
        else
            dataArray2 = cell(2);
        end
        if(fileID3 > -1)
            dataArray3 = textscan(fileID3, formatSpec, 'Delimiter', delimiter, 'EmptyValue' ,NaN,'HeaderLines' ,startRow-1, 'ReturnOnError', false);
        else
            dataArray3 = cell(2);
        end
        if(fileID4 > -1)
            dataArray4 = textscan(fileID4, formatSpec, 'Delimiter', delimiter, 'EmptyValue' ,NaN,'HeaderLines' ,startRow-1, 'ReturnOnError', false);
        else
            dataArray4 = cell(2);
        end

        %% Close the text file.
        if(fileID1 > -1)
            fclose(fileID1);
        end
        if(fileID2 > -1)
            fclose(fileID2);
        end
        if(fileID3 > -1)
            fclose(fileID3);
        end
        if(fileID4 > -1)
            fclose(fileID4);
        end


        %% Post processing for unimportable data.
        % No unimportable data rules were applied during the import, so no post
        % processing code is included. To generate code which works for
        % unimportable data, select unimportable cells in a file and regenerate the
        % script.

        %% Allocate imported array to column variable names
        % Transforming in matrix
        rcvd_pkt = [dataArray1{:, 1} dataArray1{:, 2}]; % array of received packets
        addressed_ca = [dataArray2{:, 1} dataArray2{:, 2}]; % array indicating time of acquired ip
        addressed_da = [dataArray3{:, 1} dataArray3{:, 2}];
        rtt = [dataArray4{:, 1} dataArray4{:, 2}];


        %% Processing data
        % Measuring delay
        if(length(rcvd_pkt) > seq_thr & length(addressed_ca) > 0) % Skipping data set if seq is below 
            % we remove retransmissions from set
            bit_mask = true(size(addressed_ca(:,1))); % bit mask
            last_time = 1; % starting point
            for idx_addr=2:length(addressed_ca(:,1)) % for all remaining points,
                if(abs(addressed_ca(idx_addr,1)-addressed_ca(last_time,1))<retransmission_thr) % If this point is within the tolerance of the last accepted point, set it as false in the bit mask;
                    bit_mask(idx_addr) = false;
                else % Otherwise, keep it and mark the last kept
                    last_time = idx_addr;
                end
            end
            addressed_ca = addressed_ca(bit_mask,:); % remove indexed values
            vecAddressChanges = [vecAddressChanges; addressed_ca(:,1)]; % timestamps of new ip addresses
            % calculating handover
            last_seq = 1;
            tmp_addr_da = [];
            tmp_delay = [];
            for idx_delay = 1:length(addressed_ca(:,1)) % iterate through all update messages
                tmp_seq = rcvd_pkt(addressed_ca(idx_delay,1) <= rcvd_pkt(:,1)); % consider all packets after update msg
                if(numel(tmp_seq) > 0) % is there any packet received after update msg
                    tmp_seq = tmp_seq(tmp_seq(:) <= (tmp_seq(1)+period_time)); % ignore  measurement if only some single packages are received and not a stream of packets
                    if(numel(tmp_seq) > min_pkt_per_period & last_seq < tmp_seq(1)) % remove duplicates
                        if(numel(addressed_da) > 0) % update msg from DA?
                            tmp_addr_da = addressed_da(addressed_da(:,1) <= tmp_seq(1));
                        end
                        tmp_addr_ca = addressed_ca(addressed_ca(:,1) <= tmp_seq(1)); % 
                        if(numel(tmp_addr_da) > 0) % address config time by data_agent
                            tmp_addr = max([tmp_addr_da(end) tmp_addr_ca(end)]);
                        else
                            tmp_addr = tmp_addr_ca(end);
                        end
                        tmp_delay = abs(tmp_seq(1) - tmp_addr);
                        if(tmp_delay < 20) % only consider handover delays not exceeding 20s 
                            vecHandoverDelay = [vecHandoverDelay tmp_delay];
                        end
                        last_seq = tmp_seq(1);
                    end
                end
            end     
        end

        % Rtt
        if(length(rtt) > 0)
            vecRttMean = [vecRttMean mean(rtt(:,2))];
            vecRttMin = [vecRttMin min(rtt(:,2))];
            vecRttMax = [vecRttMax max(rtt(:,2))];
        end

        %% Clear temporary variables
        clearvars filename1 filename2 filename3 filename4 delimiter startRow formatSpec fileID1 fileID2 fileID3 fileID4 dataArray1 dataArray2 dataArray3 dataArray4 ans;
        clearvars rcvd_pkt addressed_ca addressed_da rtt;
        clearvars tmp_delay tmp_off last_elem idx_run idx_on last_seq bit_mask idx_delay idx_addr last_seq last_time tmp_seq tmp_addr tmp_addr_ca tmp_addr_da;
    end

    %% Processing data
    if(numel(vecHandoverDelay) > 0) % Transforming vector to array
        stat = [mean(vecHandoverDelay) std(vecHandoverDelay) var(vecHandoverDelay) min(vecHandoverDelay) max(vecHandoverDelay)];
    else
        stat = zeros(1,5);
    end
    stat = [stat; mean(vecRttMean) std(vecRttMean) var(vecRttMean) min(vecRttMean) max(vecRttMean)];

    %% Plot data
    output_folder = [main_output_folder '/' net_protocol];
    if ~exist(output_folder, 'dir')
      mkdir(output_folder);
    end
    output_folder = [output_folder '/' communication];
    if ~exist(output_folder, 'dir')
      mkdir(output_folder);
    end
    output_folder = [output_folder '/' trans_protocol{idx_app,1}];
    if ~exist(output_folder, 'dir')
      mkdir(output_folder);
    end

    if(numel(vecHandoverDelay) > 0 & PLOT)
        idx_plot = 0;
        % Handover Delay
        idx_plot = idx_plot + 1;
        figure(idx_plot); clf;
        boxplot(vecHandoverDelay);
        ylabel('Time [s]');
        xlabel('Delay');
        grid on;
        % title('Boxplot of handover delay');
        if(CREATE_TEX)
            matlab2tikz([output_folder '/ho_boxplot_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
        end
        %
        idx_plot = idx_plot + 1;
        figure(idx_plot); clf;
        hist(vecHandoverDelay);
        xlabel('Time [s]');
        ylabel('Number of Observations');
        grid on;
        % title('Histogram of Handover Delay');
        if(CREATE_TEX)
            matlab2tikz([output_folder '/ho_hist_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
        end
        %
        idx_plot = idx_plot + 1;
        figure(idx_plot); clf;
        ecdf(vecHandoverDelay);
        xlabel('Time [s]');
        ylabel('P(X \leq x)');
        grid on;
        % title('Cumulative Distribution Function of Handover Delay');
        if(CREATE_TEX)
            matlab2tikz([output_folder '/ho_ecdf_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
        end
        % plot
        idx_plot = idx_plot + 1;
        figure(idx_plot); clf;
        stem(vecHandoverDelay);
        xlabel('Time [s]');
        grid on;
        title('Handover delay');
        if(CREATE_TEX)
            matlab2tikz([output_folder '/ho_stem_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
        end
        % plot
        idx_plot = idx_plot + 1;
        figure(idx_plot); clf;
        hold on;
        scatter(1:numel(vecRttMax),vecRttMax,'+');
        scatter(1:numel(vecRttMean),vecRttMean);
        scatter(1:numel(vecRttMin),vecRttMin,'.');
        legend('Max','Mean','Min');
        xlabel('Simulation runs');
        ylabel('Time [s]');
        grid on;
        title('RTT');
        hold off;
        if(CREATE_TEX)
            matlab2tikz([output_folder '/rtt_scatter_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
        end
    end

    %% Print data
    fprintf(['*** Handover delay: ' net_protocol '_' communication '_' trans_protocol{idx_app,1} ' ***\n']);
    fprintf('Mean: %1.4f\n',stat(1,1));
    fprintf('Std: %1.4f\n', stat(1,2));
    fprintf('Var: %1.4f\n', stat(1,3));
    fprintf('Min: %1.4f\n', stat(1,4));
    fprintf('Max: %1.4f\n', stat(1,5));
    fprintf('*** RTT\n');
    fprintf('Mean: %1.4f\n',stat(2,1));
    fprintf('Std: %1.4f\n', stat(2,2));
    fprintf('Var: %1.4f\n', stat(2,3));
    fprintf('Min: %1.4f\n', stat(2,4));
    fprintf('Max: %1.4f\n', stat(2,5));
    columnLabels = {'Name', 'Mean', '\sigma', 'Var', 'Min', 'Max'};
    rowLabels = {['Delay ' net_protocol], 'RTT'};
    if(CREATE_TEX)
        matrix2latex(stat, [output_folder '/rtt_tab.tex'], 'rowLabels', rowLabels, 'columnLabels', columnLabels, 'alignment', 'c', 'format', '%-6.4f', 'size', 'tiny');
    end

    var_tmp = genvarname(['vecHandoverDelay_ ' net_protocol '_' communication '_' trans_protocol{idx_app,1}]);
    eval([var_tmp '= vecHandoverDelay;']);
    var_tmp = genvarname(['vecRttMean_ ' net_protocol '_' communication '_' trans_protocol{idx_app,1}]);
    eval([var_tmp '= vecRttMean;']);
    clearvars stat columnLabels rowLabels output_folder var_tmp;
end
clearvars seq_thr retransmission_thr min_pkt_per_period period_time idx_plot main_output_folder;
clearvars communication meas_folder net_protocol output_folder result_folder sim_time N output_folder;
clearvars app_size idx_app trans_protocol vecAddressChanges vecHandoverDelay vecRttMax vecRttMean vecRttMin;