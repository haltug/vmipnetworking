CALC = 0;
if(CALC)
%     clear all;
    close all;
    PLOT = 1; % Enables plots in sub script
    CREATE_TEX = 1; % Enables tex generation in sub scrips
    % movenet
    run frankfurt_urban/eval_movenet_1_to_1.m
    % mipv6
    run frankfurt_urban_mipv6/eval_mobileip_1_to_1.m
    % multi
    run frankfurt_urban_multi_radio/eval_movenet_multi_1_to_1.m
end
close all;
 
%% Plot data
CREATE_PLOT = 1;
idx_plot = 0;
main_output_folder = '/home/halis/Halis_MSc_Doc/gfx/results/handover'; 
communication = 'plot_1_to_1';
output_folder = [main_output_folder '/' communication];
if ~exist(output_folder, 'dir')
  mkdir(output_folder);
end

%%  Handover Delay: TCP
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
ax1 = subplot(1,6,1);
boxplot(vecHandoverDelay_Mobileipv6_1_to_1_TCP_CN_to_MA);
grid on;
ylim(ax1,[0.000001 30]);
set(gca,'YScale','log');
ylabel('Time [s]');
xlabel(['Delay TCP CN->VN' char(10) 'MobileIPv6']); 
ax2 = subplot(1,6,2);
boxplot(vecHandoverDelay_Movenet_1_to_1_TCP_CN_to_MA);
grid on;
ylim(ax2,[0.000001 30]);
set(gca,'YScale','log');
ylabel('Time [s]');
xlabel(['Delay TCP CN->VN' char(10) 'MoVeNet']);
ax3 = subplot(1,6,3);
boxplot(vecHandoverDelay_Movenet_multi_1_to_1_TCP_CN_to_MA);
grid on;
ylim(ax3,[0.000001 30]);
set(gca,'YScale','log');
ylabel('Time [s]');
xlabel(['Delay TCP CN->VN' char(10) 'MoVeNet Multi-homing']);
grid on;
ax4 = subplot(1,6,4);
boxplot(vecHandoverDelay_Mobileipv6_1_to_1_TCP_MA_to_CN);
grid on;
set(gca,'YScale','log');
ylim(ax4,[0.000001 30]);
ylabel('Time [s]');
xlabel(['Delay TCP VN->CN' char(10) 'MobileIPv6']);
ax5 = subplot(1,6,5);
boxplot(vecHandoverDelay_Movenet_1_to_1_TCP_MA_to_CN);
grid on;
set(gca,'YScale','log');
ylim(ax5,[0.000001 30]);
ylabel('Time [s]');
xlabel(['Delay TCP VN->CN' char(10) 'MoVeNet']);
ax6 = subplot(1,6,6);
boxplot(vecHandoverDelay_Movenet_multi_1_to_1_TCP_MA_to_CN);
grid on;
ylim(ax6,[0.000001 30]);
set(gca,'YScale','log');
ylabel('Time [s]');
xlabel(['Delay TCP VN->CN' char(10) 'MoVeNet Multi-homing']);
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_1_to_1_TCP_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%%  Handover Delay: UDP
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
ax1 = subplot(1,6,1);
boxplot(vecHandoverDelay_Mobileipv6_1_to_1_UDP_CN_to_MA);
grid on;
ylim(ax1,[0.000001 30]);
set(gca,'YScale','log');
ylabel('Time [s]');
xlabel(['Delay UDP CN->VN' char(10) 'MobileIPv6']); 
ax2 = subplot(1,6,2);
boxplot(vecHandoverDelay_Movenet_1_to_1_UDP_CN_to_MA);
grid on;
set(gca,'YScale','log');
ylim(ax2,[0.000001 30]);
ylabel('Time [s]');
xlabel(['Delay UDP CN->VN' char(10) 'MoVeNet']);
ax3 = subplot(1,6,3);
boxplot(vecHandoverDelay_Movenet_multi_1_to_1_UDP_CN_to_MA);
grid on;
ylim(ax3,[0.000001 30]);
set(gca,'YScale','log');
ylabel('Time [s]');
xlabel(['Delay UDP CN->VN' char(10) 'MoVeNet Multi-homing']);
ax4 = subplot(1,6,4);
boxplot(vecHandoverDelay_Mobileipv6_1_to_1_UDP_MA_to_CN);
grid on;
ylim(ax4,[0.000001 30]);
set(gca,'YScale','log');
ylabel('Time [s]');
xlabel(['Delay UDP VN->CN' char(10) 'MobileIPv6']);
ax5 = subplot(1,6,5);
boxplot(vecHandoverDelay_Movenet_1_to_1_UDP_MA_to_CN);
grid on;
set(gca,'YScale','log');
ylim(ax5,[0.000001 30]);
ylabel('Time [s]');
xlabel(['Delay UDP VN->CN' char(10) 'MoVeNet']);
ax6 = subplot(1,6,6);
boxplot(vecHandoverDelay_Movenet_multi_1_to_1_UDP_MA_to_CN);
grid on;
ylim(ax6,[0.000001 30]);
set(gca,'YScale','log');
ylabel('Time [s]');
xlabel(['Delay UDP VN->CN' char(10) 'MoVeNet Multi-homing']);
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_1_to_1_UDP_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%% RTT TCP
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
ax1 = subplot(1,6,1);
boxplot(vecRttMean_Mobileipv6_1_to_1_TCP_CN_to_MA);
grid on;
ylim(ax1,[0 1]);
ylabel('Time [s]');
xlabel(['RTT TCP CN->VN' char(10) 'MobileIPv6']); 
ax2 = subplot(1,6,2);
boxplot(vecRttMean_Movenet_1_to_1_TCP_CN_to_MA);
grid on;
ylim(ax2,[0 1]);
ylabel('Time [s]');
xlabel(['RTT TCP CN->VN' char(10) 'MoVeNet']);
ax3 = subplot(1,6,3);
boxplot(vecRttMean_Movenet_multi_1_to_1_TCP_CN_to_MA);
grid on;
ylim(ax3,[0 1]);
ylabel('Time [s]');
xlabel(['RTT TCP CN->VN' char(10) 'MoVeNet Multi-homing']);
ax4 = subplot(1,6,4);
boxplot(vecRttMean_Mobileipv6_1_to_1_TCP_MA_to_CN);
grid on;
ylim(ax4,[0 0.1]);
ylabel('Time [s]');
xlabel(['RTT TCP VN->CN' char(10) 'MobileIPv6']);
ax5 = subplot(1,6,5);
boxplot(vecRttMean_Movenet_1_to_1_TCP_MA_to_CN);
grid on;
ylim(ax5,[0 0.1]);
ylabel('Time [s]');
xlabel(['RTT TCP VN->CN' char(10) 'MoVeNet']);
ax6 = subplot(1,6,6);
boxplot(vecRttMean_Movenet_multi_1_to_1_TCP_MA_to_CN);
grid on;
ylim(ax6,[0 0.1]);
ylabel('Time [s]');
xlabel(['RTT TCP VN->CN' char(10) 'MoVeNet Multi-homing']);
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_1_to_1_TCP_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%% RTT UDP
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
ax1 = subplot(1,6,1);
boxplot(vecRttMean_Mobileipv6_1_to_1_UDP_CN_to_MA);
grid on;
ylim(ax1,[0.01 0.04]);
ylabel('Time [s]');
xlabel(['RTT UDP CN->VN' char(10) 'MobileIPv6']); 
ax2 = subplot(1,6,2);
boxplot(vecRttMean_Movenet_1_to_1_UDP_CN_to_MA);
grid on;
ylim(ax2,[0.01 0.04]);
ylabel('Time [s]');
xlabel(['RTT UDP CN->VN' char(10) 'MoVeNet']);
ax3 = subplot(1,6,3);
boxplot(vecRttMean_Movenet_multi_1_to_1_UDP_CN_to_MA);
grid on;
ylim(ax3,[0.01 0.04]);
ylabel('Time [s]');
xlabel(['RTT UDP CN->VN' char(10) 'MoVeNet Multi-homing']);
ax4 = subplot(1,6,4);
boxplot(vecRttMean_Mobileipv6_1_to_1_UDP_MA_to_CN);
grid on;
ylim(ax4,[0 0.05]);
ylabel('Time [s]');
xlabel(['RTT UDP VN->CN' char(10) 'MobileIPv6']);
ax5 = subplot(1,6,5);
boxplot(vecRttMean_Movenet_1_to_1_UDP_MA_to_CN);
grid on;
ylim(ax5,[0 0.05]);
ylabel('Time [s]');
xlabel(['RTT UDP VN->CN' char(10) 'MoVeNet']);
ax6 = subplot(1,6,6);
boxplot(vecRttMean_Movenet_multi_1_to_1_UDP_MA_to_CN);
grid on;
ylim(ax6,[0 0.05]);
ylabel('Time [s]');
xlabel(['RTT UDP VN->CN' char(10) 'MoVeNet Multi-homing']);
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_1_to_1_UDP_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

clear ax1 ax2 ax3 ax4 ax5 ax6 idx_plot CREATE_PLOT CREATE_TEX PLOT;
%