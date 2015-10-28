CALC = 0;
if(CALC)
%     clear all;
    close all;
    PLOT = 0; % Enables plots in sub script
    CREATE_TEX = 0; % Enables tex generation in sub scrips
    % movenet
    run frankfurt_urban_n_vehicles/eval_movenet_n_to_1.m
    % mipv6
    run frankfurt_urban_n_vehicles_mipv6/eval_mobileip_n_to_1.m
    % multi
%     run frankfurt_urban_n_vehicles_multi_radio/eval_movenet_multi_n_to_1.m
end
close all;
 
%% Plot data
MODE = 0;
CREATE_PLOT = 1;
N_vehicle = 8;
idx_plot = 0;
main_output_folder = '/home/halis/Halis_MSc_Doc/gfx/results/handover'; 
communication = 'plot_n_to_1';
output_folder = [main_output_folder '/' communication];
if ~exist(output_folder, 'dir')
  mkdir(output_folder);
end
v = [];
g = [];

%%  Handover Delay: TCP CN -> VN
idx_plot = idx_plot + 1; %1
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellHandoverDelay_Mobileipv6_n_to_1_TCP_CN_to_MA{1,idx_veh}];
    g = [g; (zeros(length(cellHandoverDelay_Mobileipv6_n_to_1_TCP_CN_to_MA{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.01 100]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_Mobileipv6_n_to_1_TCP_CN_to_MA_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
%
v = [];
g = [];
idx_plot = idx_plot + 1; %2
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellHandoverDelay_Movenet_n_to_1_TCP_CN_to_MA{1,idx_veh}];
    g = [g; (zeros(length(cellHandoverDelay_Movenet_n_to_1_TCP_CN_to_MA{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.01 100]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_Movenet_n_to_1_TCP_CN_to_MA_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
v = [];
g = [];
%%  Handover Delay: TCP VN -CN
idx_plot = idx_plot + 1; %3
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellHandoverDelay_Mobileipv6_n_to_1_TCP_MA_to_CN{1,idx_veh}];
    g = [g; (zeros(length(cellHandoverDelay_Mobileipv6_n_to_1_TCP_MA_to_CN{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.001 100]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_Mobileipv6_n_to_1_TCP_MA_to_CN_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
%
v = [];
g = [];
idx_plot = idx_plot + 1; %4
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellHandoverDelay_Movenet_n_to_1_TCP_MA_to_CN{1,idx_veh}];
    g = [g; (zeros(length(cellHandoverDelay_Movenet_n_to_1_TCP_MA_to_CN{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.001 100]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_Movenet_n_to_1_TCP_MA_to_CN_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%%  Handover Delay: UDP CN -> VN
v = [];
g = [];
idx_plot = idx_plot + 1; %5
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellHandoverDelay_Mobileipv6_n_to_1_UDP_CN_to_MA{1,idx_veh}];
    g = [g; (zeros(length(cellHandoverDelay_Mobileipv6_n_to_1_UDP_CN_to_MA{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.001 10]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_Mobileipv6_n_to_1_UDP_CN_to_MA_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
%
v = [];
g = [];
idx_plot = idx_plot + 1; %6
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellHandoverDelay_Movenet_n_to_1_UDP_CN_to_MA{1,idx_veh}];
    g = [g; (zeros(length(cellHandoverDelay_Movenet_n_to_1_UDP_CN_to_MA{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.001 10]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_Movenet_n_to_1_UDP_CN_to_MA_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%%  Handover Delay: UDP VN -CN
v = [];
g = [];
idx_plot = idx_plot + 1; %7
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellHandoverDelay_Mobileipv6_n_to_1_UDP_MA_to_CN{1,idx_veh}];
    g = [g; (zeros(length(cellHandoverDelay_Mobileipv6_n_to_1_UDP_MA_to_CN{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.001 100]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_Mobileipv6_n_to_1_UDP_MA_to_CN_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
%
v = [];
g = [];
idx_plot = idx_plot + 1; %8
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellHandoverDelay_Movenet_n_to_1_UDP_MA_to_CN{1,idx_veh}];
    g = [g; (zeros(length(cellHandoverDelay_Movenet_n_to_1_UDP_MA_to_CN{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.001 100]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_Movenet_n_to_1_UDP_MA_to_CN_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%===============================================================================
%%  RTT: TCP CN -> VN
v = [];
g = [];
idx_plot = idx_plot + 1; %9
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellRttMean_Mobileipv6_n_to_1_TCP_CN_to_MA{1,idx_veh}];
    g = [g; (zeros(length(cellRttMean_Mobileipv6_n_to_1_TCP_CN_to_MA{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.1 10]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_Mobileipv6_n_to_1_TCP_CN_to_MA_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
%
v = [];
g = [];
idx_plot = idx_plot + 1; %10
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellRttMean_Movenet_n_to_1_TCP_CN_to_MA{1,idx_veh}];
    g = [g; (zeros(length(cellRttMean_Movenet_n_to_1_TCP_CN_to_MA{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.1 10]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_Movenet_n_to_1_TCP_CN_to_MA_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%%  RTT: TCP VN -CN
v = [];
g = [];
idx_plot = idx_plot + 1; %11
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellRttMean_Mobileipv6_n_to_1_TCP_MA_to_CN{1,idx_veh}];
    g = [g; (zeros(length(cellRttMean_Mobileipv6_n_to_1_TCP_MA_to_CN{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.1 1]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_Mobileipv6_n_to_1_TCP_MA_to_CN_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
%
v = [];
g = [];
idx_plot = idx_plot + 1; %12
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellRttMean_Movenet_n_to_1_TCP_MA_to_CN{1,idx_veh}];
    g = [g; (zeros(length(cellRttMean_Movenet_n_to_1_TCP_MA_to_CN{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.1 1]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_Movenet_n_to_1_TCP_MA_to_CN_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%%  RTT: UDP CN -> VN
v = [];
g = [];
idx_plot = idx_plot + 1; %13
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellRttMean_Mobileipv6_n_to_1_UDP_CN_to_MA{1,idx_veh}];
    g = [g; (zeros(length(cellRttMean_Mobileipv6_n_to_1_UDP_CN_to_MA{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.01 1]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_Mobileipv6_n_to_1_UDP_CN_to_MA_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
%
v = [];
g = [];
idx_plot = idx_plot + 1; %14
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellRttMean_Movenet_n_to_1_UDP_CN_to_MA{1,idx_veh}];
    g = [g; (zeros(length(cellRttMean_Movenet_n_to_1_UDP_CN_to_MA{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.01 1]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_Movenet_n_to_1_UDP_CN_to_MA_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%%  RTT: UDP VN -CN
v = [];
g = [];
idx_plot = idx_plot + 1; %15
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellRttMean_Mobileipv6_n_to_1_UDP_MA_to_CN{1,idx_veh}];
    g = [g; (zeros(length(cellRttMean_Mobileipv6_n_to_1_UDP_MA_to_CN{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.01 10]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_Mobileipv6_n_to_1_UDP_MA_to_CN_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
%
v = [];
g = [];
idx_plot = idx_plot + 1; %16
figure(idx_plot); clf;
for idx_veh = 1:N_vehicle
    v = [v cellRttMean_Movenet_n_to_1_UDP_MA_to_CN{1,idx_veh}];
    g = [g; (zeros(length(cellRttMean_Movenet_n_to_1_UDP_MA_to_CN{1,idx_veh}),1)+idx_veh);];
end
boxplot(v,g);
boxplotLabel(N_vehicle,{'1','2','3','4','5','6','7','8'}, '');
grid on;
ylim([0.01 10]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_Movenet_n_to_1_UDP_MA_to_CN_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
clear ax1 ax2 ax3 ax4 ax5 ax6 idx_plot CREATE_PLOT CREATE_TEX PLOT communication idx_veh main_output_folder MODE N_vehicle output_folder;
% if(MODE) % 1 = single mode
    %%  Handover Delay: TCP CN -> VN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax1 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Mobileipv6_n_to_1_TCP_CN_to_MA{1,idx_veh});
%         % ylim(ax1,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['Delay TCP CN->VN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMobileipv6_n_to_1_TCP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax2 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Movenet_n_to_1_TCP_CN_to_MA{1,idx_veh});
%         % ylim(ax2,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['Delay TCP CN->VN' char(10) num2str(idx_veh) ': MoVeNet']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMovenet_n_to_1_TCP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax3 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Movenet_multi_n_to_1_TCP_CN_to_MA{1,idx_veh});
%     %     ylim(ax3,[0 0.1]);
%         ylabel('Time [s]');
%         xlabel(['Delay TCP CN->VN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMovenet_multi_n_to_1_TCP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end

    %%  Handover Delay: TCP VN -> CN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax1 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Mobileipv6_n_to_1_TCP_MA_to_CN{1,idx_veh});
%         % ylim(ax1,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['Delay TCP VN->CN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMobileipv6_n_to_1_TCP_MA_to_CN_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax2 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Movenet_n_to_1_TCP_MA_to_CN{1,idx_veh});
%         % ylim(ax2,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['Delay TCP VN->CN' char(10) num2str(idx_veh) ': MoVeNet']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMovenet_n_to_1_TCP_MA_to_CN_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax3 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Movenet_multi_n_to_1_TCP_MA_to_CN{1,idx_veh});
%     %     ylim(ax3,[0 0.1]);
%         ylabel('Time [s]');
%         xlabel(['Delay TCP VN->CN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMovenet_multi_n_to_1_TCP_MA_to_CN_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end    

    %%  Handover Delay: UDP CN -> VN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax1 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Mobileipv6_n_to_1_UDP_CN_to_MA{1,idx_veh});
%         % ylim(ax1,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['Delay UDP CN->VN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMobileipv6_n_to_1_UDP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax2 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Movenet_n_to_1_UDP_CN_to_MA{1,idx_veh});
%         % ylim(ax2,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['Delay UDP CN->VN' char(10) num2str(idx_veh) ': MoVeNet']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMovenet_n_to_1_UDP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax3 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Movenet_multi_n_to_1_UDP_CN_to_MA{1,idx_veh});
%     %     ylim(ax3,[0 0.1]);
%         ylabel('Time [s]');
%         xlabel(['Delay UDP CN->VN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMovenet_multi_n_to_1_UDP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end

    %%  Handover Delay: UDP VN -> CN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax1 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Mobileipv6_n_to_1_UDP_MA_to_CN{1,idx_veh});
%         % ylim(ax1,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['Delay UDP VN->CN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/ho_Mobileipv6_n_to_1_UDP_MA_to_CN_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax2 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Movenet_n_to_1_UDP_MA_to_CN{1,idx_veh});
%         % ylim(ax2,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['Delay UDP VN->CN' char(10) num2str(idx_veh) ': MoVeNet']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMovenet_n_to_1_UDP_MA_to_CN.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax3 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Movenet_multi_n_to_1_UDP_MA_to_CN{1,idx_veh});
%     %     ylim(ax3,[0 0.1]);
%         ylabel('Time [s]');
%         xlabel(['Delay UDP VN->CN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMovenet_multi_n_to_1_UDP_MA_to_CN_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end

%     %%  RTT: TCP CN -> VN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax1 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Mobileipv6_n_to_1_TCP_CN_to_MA{1,idx_veh});
%         % ylim(ax1,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['RTT TCP CN->VN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMobileipv6_n_to_1_TCP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax2 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Movenet_n_to_1_TCP_CN_to_MA{1,idx_veh});
%         % ylim(ax2,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['RTT TCP CN->VN' char(10) num2str(idx_veh) ': MoVeNet']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMovenet_n_to_1_TCP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
% %     idx_plot = idx_plot + 1;
% %     figure(idx_plot); clf;
% %     for idx_veh = 1:N_vehicle
% %         grid on;
% %         ax3 = subplot(1,N_vehicle,idx_veh);
% %         boxplot(cellRttMean_Movenet_multi_n_to_1_TCP_CN_to_MA{1,idx_veh});
% %     %     ylim(ax3,[0 0.1]);
% %         ylabel('Time [s]');
% %         xlabel(['RTT TCP CN->VN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
% %         if(CREATE_PLOT)
% %             matlab2tikz([output_folder '/rttMovenet_multi_n_to_1_TCP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
% %         end
% %     end
% 
%     %%  RTT: TCP VN -> CN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax1 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Mobileipv6_n_to_1_TCP_MA_to_CN{1,idx_veh});
%         % ylim(ax1,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['RTT TCP VN->CN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMobileipv6_n_to_1_TCP_MA_to_CN_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax2 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Movenet_n_to_1_TCP_MA_to_CN{1,idx_veh});
%         % ylim(ax2,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['RTT TCP VN->CN' char(10) num2str(idx_veh) ': MoVeNet']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMovenet_n_to_1_TCP_MA_to_CN_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
% %     idx_plot = idx_plot + 1;
% %     figure(idx_plot); clf;
% %     for idx_veh = 1:N_vehicle
% %         grid on;
% %         ax3 = subplot(1,N_vehicle,idx_veh);
% %         boxplot(cellRttMean_Movenet_multi_n_to_1_TCP_MA_to_CN{1,idx_veh});
% %     %     ylim(ax3,[0 0.1]);
% %         ylabel('Time [s]');
% %         xlabel(['RTT TCP VN->CN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
% %         if(CREATE_PLOT)
% %             matlab2tikz([output_folder '/rttMovenet_multi_n_to_1_TCP_MA_to_CN_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
% %         end
% %     end    
% 
%     %%  RTT: UDP CN -> VN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax1 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Mobileipv6_n_to_1_UDP_CN_to_MA{1,idx_veh});
%         % ylim(ax1,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['RTT UDP CN->VN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMobileipv6_n_to_1_UDP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax2 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Movenet_n_to_1_UDP_CN_to_MA{1,idx_veh});
%         % ylim(ax2,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['RTT UDP CN->VN' char(10) num2str(idx_veh) ': MoVeNet']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMovenet_n_to_1_UDP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
% %     idx_plot = idx_plot + 1;
% %     figure(idx_plot); clf;
% %     for idx_veh = 1:N_vehicle
% %         grid on;
% %         ax3 = subplot(1,N_vehicle,idx_veh);
% %         boxplot(cellRttMean_Movenet_multi_n_to_1_UDP_CN_to_MA{1,idx_veh});
% %     %     ylim(ax3,[0 0.1]);
% %         ylabel('Time [s]');
% %         xlabel(['RTT UDP CN->VN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
% %         if(CREATE_PLOT)
% %             matlab2tikz([output_folder '/rttMovenet_multi_n_to_1_UDP_CN_to_MA_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
% %         end
% %     end
% 
%     %%  RTT: UDP VN -> CN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax1 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Mobileipv6_n_to_1_UDP_MA_to_CN{1,idx_veh});
%         % ylim(ax1,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['RTT UDP VN->CN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMobileipv6_n_to_1_UDP_MA_to_CN_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         grid on;
%         ax2 = subplot(1,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Movenet_n_to_1_UDP_MA_to_CN{1,idx_veh});
%         % ylim(ax2,[0 20]);
%         ylabel('Time [s]');
%         xlabel(['RTT UDP VN->CN' char(10) num2str(idx_veh) ': MoVeNet']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMovenet_n_to_1_UDP_MA_to_CN.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
% %     idx_plot = idx_plot + 1;
% %     figure(idx_plot); clf;
% %     for idx_veh = 1:N_vehicle
% %         grid on;
% %         ax3 = subplot(1,N_vehicle,idx_veh);
% %         boxplot(cellRttMean_Movenet_multi_n_to_1_UDP_MA_to_CN{1,idx_veh});
% %     %     ylim(ax3,[0 0.1]);
% %         ylabel('Time [s]');
% %         xlabel(['RTT UDP VN->CN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
% %         if(CREATE_PLOT)
% %             matlab2tikz([output_folder '/rttMovenet_multi_n_to_1_UDP_MA_to_CN_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
% %         end
% %     end
% else % compare mode
% %     ==============================================================================
% %     ==============================================================================
% %     ==============================================================================
% %     ==============================================================================
% %     ==============================================================================
% %     ==============================================================================
% %     ==============================================================================
% %     ==============================================================================
% %     ==============================================================================
%     %%  Handover Delay: TCP CN -> VN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         ax1 = subplot(2,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Mobileipv6_n_to_1_TCP_CN_to_MA{1,idx_veh});
%         grid on;
%         ylim(ax1,[0.01 100]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['Delay TCP CN->VN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         ax2 = subplot(2,N_vehicle,idx_veh+N_vehicle);
%         boxplot(cellHandoverDelay_Movenet_n_to_1_TCP_CN_to_MA{1,idx_veh});
%         grid on;
%         ylim(ax2,[0.01 100]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['Delay TCP CN->VN' char(10) num2str(idx_veh) ': MoVeNet']);
% %         ax3 = subplot(3,N_vehicle,idx_veh+N_vehicle*2);
% %         boxplot(cellHandoverDelay_Movenet_multi_n_to_1_TCP_CN_to_MA{1,idx_veh});
% %         grid on;
% %         ylim(ax3,[0 10]);
% %         ylabel('Time [s]');
% %         xlabel(['Delay TCP CN->VN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMobileipv6_n_to_1_TCP_CN_to_MA_fig_comp.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
% 
%     %%  Handover Delay: TCP VN -> CN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         ax1 = subplot(2,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Mobileipv6_n_to_1_TCP_MA_to_CN{1,idx_veh});
%         grid on;
%         ylim(ax1,[0.01 100]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['Delay TCP VN->CN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         ax2 = subplot(2,N_vehicle,idx_veh+N_vehicle);
%         boxplot(cellHandoverDelay_Movenet_n_to_1_TCP_MA_to_CN{1,idx_veh});
%         grid on;
%         ylim(ax2,[0.01 100]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['Delay TCP VN->CN' char(10) num2str(idx_veh) ': MoVeNet']);
% %         ax3 = subplot(3,N_vehicle,idx_veh+N_vehicle*2);
% %         boxplot(cellHandoverDelay_Movenet_multi_n_to_1_TCP_MA_to_CN{1,idx_veh});
% %         grid on;
% %         ylim(ax3,[0 10]);
% %         ylabel('Time [s]');
% %         xlabel(['Delay TCP VN->CN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMobileipv6_n_to_1_TCP_MA_to_CN_fig_comp.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end   
% 
%     %%  Handover Delay: UDP CN -> VN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         ax1 = subplot(2,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Mobileipv6_n_to_1_UDP_CN_to_MA{1,idx_veh});
%         grid on;
%         ylim(ax1,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['Delay UDP CN->VN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         ax2 = subplot(2,N_vehicle,idx_veh+N_vehicle);
%         boxplot(cellHandoverDelay_Movenet_n_to_1_UDP_CN_to_MA{1,idx_veh});
%         grid on;
%         ylim(ax2,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['Delay UDP CN->VN' char(10) num2str(idx_veh) ': MoVeNet']);
% %         ax3 = subplot(3,N_vehicle,idx_veh+N_vehicle*2);
% %         boxplot(cellHandoverDelay_Movenet_multi_n_to_1_UDP_CN_to_MA{1,idx_veh});
% %         grid on;
% %         ylim(ax3,[0 0.5]);
% %         ylabel('Time [s]');
% %         xlabel(['Delay UDP CN->VN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);        
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/hoMobileipv6_n_to_1_UDP_CN_to_MA_fig_comp.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
% 
%     %%  Handover Delay: UDP VN -> CN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         ax1 = subplot(2,N_vehicle,idx_veh);
%         boxplot(cellHandoverDelay_Mobileipv6_n_to_1_UDP_MA_to_CN{1,idx_veh});
%         grid on;
%         ylim(ax1,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['Delay UDP VN->CN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         ax2 = subplot(2,N_vehicle,idx_veh+N_vehicle);
%         boxplot(cellHandoverDelay_Movenet_n_to_1_UDP_MA_to_CN{1,idx_veh});
%         grid on;
%         ylim(ax2,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['Delay UDP VN->CN' char(10) num2str(idx_veh) ': MoVeNet']);
% %         ax3 = subplot(3,N_vehicle,idx_veh+N_vehicle*2);
% %         boxplot(cellHandoverDelay_Movenet_multi_n_to_1_UDP_MA_to_CN{1,idx_veh});
% %         grid on;
% %         ylim(ax3,[0 0.5]);
% %         ylabel('Time [s]');
% %         xlabel(['Delay UDP VN->CN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/ho_Mobileipv6_n_to_1_UDP_MA_to_CN_fig_comp.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
% 
% 
%     %%  RTT: TCP CN -> VN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         ax1 = subplot(2,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Mobileipv6_n_to_1_TCP_CN_to_MA{1,idx_veh});
%         grid on;
%         ylim(ax1,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['RTT TCP CN->VN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         ax2 = subplot(2,N_vehicle,idx_veh+N_vehicle);
%         boxplot(cellRttMean_Movenet_n_to_1_TCP_CN_to_MA{1,idx_veh});
%         grid on;
%         ylim(ax2,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['RTT TCP CN->VN' char(10) num2str(idx_veh) ': MoVeNet']);
% %         ax3 = subplot(3,N_vehicle,idx_veh+N_vehicle*2);
% %         boxplot(cellRttMean_Movenet_multi_n_to_1_TCP_CN_to_MA{1,idx_veh});
% %         grid on;
% %         ylim(ax3,[0 2]);
% %         ylabel('Time [s]');
% %         xlabel(['RTT TCP CN->VN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMobileipv6_n_to_1_TCP_CN_to_MA_fig_comp.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
% 
%     %%  RTT: TCP VN -> CN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         ax1 = subplot(2,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Mobileipv6_n_to_1_TCP_MA_to_CN{1,idx_veh});
%         grid on;
%         ylim(ax1,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['RTT TCP VN->CN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         ax2 = subplot(2,N_vehicle,idx_veh+N_vehicle);
%         boxplot(cellRttMean_Movenet_n_to_1_TCP_MA_to_CN{1,idx_veh});
%         grid on;
%         ylim(ax2,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['RTT TCP VN->CN' char(10) num2str(idx_veh) ': MoVeNet']);
% %         ax3 = subplot(2,N_vehicle,idx_veh+N_vehicle*2);
% %         boxplot(cellRttMean_Movenet_multi_n_to_1_TCP_MA_to_CN{1,idx_veh});
% %         grid on;
% %         ylim(ax3,[0 0.25]);
% %         ylabel('Time [s]');
% %         xlabel(['RTT TCP VN->CN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMobileipv6_n_to_1_TCP_MA_to_CN_fig_comp.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end  
% 
%     %%  RTT: UDP CN -> VN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         ax1 = subplot(2,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Mobileipv6_n_to_1_UDP_CN_to_MA{1,idx_veh});
%         grid on;
%         ylim(ax1,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['RTT UDP CN->VN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         ax2 = subplot(2,N_vehicle,idx_veh+N_vehicle);
%         boxplot(cellRttMean_Movenet_n_to_1_UDP_CN_to_MA{1,idx_veh});
%         grid on;
%         ylim(ax2,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['RTT UDP CN->VN' char(10) num2str(idx_veh) ': MoVeNet']);
% %         ax3 = subplot(2,N_vehicle,idx_veh+N_vehicle*2);
% %         boxplot(cellRttMean_Movenet_multi_n_to_1_UDP_CN_to_MA{1,idx_veh});
% %         grid on;
% %         ylim(ax3,[0 0.2]);
% %         ylabel('Time [s]');
% %         xlabel(['RTT UDP CN->VN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMobileipv6_n_to_1_UDP_CN_to_MA_fig_comp.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
% 
%     %%  RTT: UDP VN -> CN
%     idx_plot = idx_plot + 1;
%     figure(idx_plot); clf;
%     for idx_veh = 1:N_vehicle
%         ax1 = subplot(2,N_vehicle,idx_veh);
%         boxplot(cellRttMean_Mobileipv6_n_to_1_UDP_MA_to_CN{1,idx_veh});
%         grid on;
%         ylim(ax1,[0.01 10]);
%         set(gca,'YScale','log');
%         ylabel('Time [s]');
%         xlabel(['RTT UDP VN->CN' char(10) num2str(idx_veh) ': MobileIPv6']);
%         ax2 = subplot(2,N_vehicle,idx_veh+N_vehicle);
%         boxplot(cellRttMean_Movenet_n_to_1_UDP_MA_to_CN{1,idx_veh});
%         grid on;
%         ylim(ax2,[0.01 10]);
%         set(gca,'YScale','log');        
%         ylabel('Time [s]');
%         xlabel(['RTT UDP VN->CN' char(10) num2str(idx_veh) ': MoVeNet']);
% %         ax3 = subplot(3,N_vehicle,idx_veh+N_vehicle*2);
% %         boxplot(cellRttMean_Movenet_multi_n_to_1_UDP_MA_to_CN{1,idx_veh});
% %         grid on;
% %         ylim(ax3,[0 1]);
% %         ylabel('Time [s]');
% %         xlabel(['RTT UDP VN->CN' char(10) num2str(idx_veh) ': MoVeNet Multi-homing']);
%         if(CREATE_PLOT)
%             matlab2tikz([output_folder '/rttMobileipv6_n_to_1_UDP_MA_to_CN_fig_comp.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
%         end
%     end
% end

%