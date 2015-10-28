CALC = 0;
if(CALC)
%     clear all;
    close all;
    PLOT = 1; % Enables plots in sub script
    CREATE_TEX = 0; % Enables tex generation in sub scrips
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
v = [];
g = [];

%%  Handover Delay: TCP CN_MA
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
v = [vecHandoverDelay_Mobileipv6_1_to_1_TCP_CN_to_MA vecHandoverDelay_Movenet_1_to_1_TCP_CN_to_MA vecHandoverDelay_Movenet_multi_1_to_1_TCP_CN_to_MA];
g = [zeros(length(vecHandoverDelay_Mobileipv6_1_to_1_TCP_CN_to_MA),1); ones(length(vecHandoverDelay_Movenet_1_to_1_TCP_CN_to_MA),1); (zeros(length(vecHandoverDelay_Movenet_multi_1_to_1_TCP_CN_to_MA),1)+2);];
% boxplot(v,g,'labels',{'MIP','MoV','MoV(MH)'});
boxplot(v,g);
boxplotLabel(3,{'Mobile IPv6','MoVeNet','MoVe-(MH)'}, '');
grid on;
ylim([0.001 100]);
set(gca,'YScale','log');
ylabel('Time [s]');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_1_to_1_TCP_CN_to_MA_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
% ECDF
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
hold on;
h1 = cdfplot(vecHandoverDelay_Mobileipv6_1_to_1_TCP_CN_to_MA);
h2 = cdfplot(vecHandoverDelay_Movenet_1_to_1_TCP_CN_to_MA);
h3 = cdfplot(vecHandoverDelay_Movenet_multi_1_to_1_TCP_CN_to_MA);
hold off;
set(h1, 'Color',[0.8,0.4,0]);
set(h2, 'Color',[0,0.4,0.8]);
set(h3, 'Color',[0.2,0.6,0.4]);
xlabel('Time [s]');
ylabel('P(X \leq x)');
title('');
grid on;
legend('Mobile IPv6','MoVeNet','MoVeNet (MH)','Location','southeast');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_1_to_1_TCP_CN_to_MA_ecdf.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%%  Handover Delay: TCP MA_CN
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
v = [vecHandoverDelay_Mobileipv6_1_to_1_TCP_MA_to_CN vecHandoverDelay_Movenet_1_to_1_TCP_MA_to_CN vecHandoverDelay_Movenet_multi_1_to_1_TCP_MA_to_CN];
g = [zeros(length(vecHandoverDelay_Mobileipv6_1_to_1_TCP_MA_to_CN),1); ones(length(vecHandoverDelay_Movenet_1_to_1_TCP_MA_to_CN),1); (zeros(length(vecHandoverDelay_Movenet_multi_1_to_1_TCP_MA_to_CN),1)+2);];
boxplot(v,g);
boxplotLabel(3,{'Mobile IPv6','MoVeNet','MoVe-(MH)'}, '');
grid on;
ylim([0.001 100]);
set(gca,'YScale','log');
ylabel('Time [s]');
% xlabel(['Delay TCP VN->CN' char(10) 'MobileIPv6']);
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_1_to_1_TCP_MA_to_CN_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
% ECDF
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
hold on;
h1 = cdfplot(vecHandoverDelay_Mobileipv6_1_to_1_TCP_MA_to_CN);
h2 = cdfplot(vecHandoverDelay_Movenet_1_to_1_TCP_MA_to_CN);
h3 = cdfplot(vecHandoverDelay_Movenet_multi_1_to_1_TCP_MA_to_CN);
hold off;
set(h1, 'Color',[0.8,0.4,0]);
set(h2, 'Color',[0,0.4,0.8]);
set(h3, 'Color',[0.2,0.6,0.4]);
xlabel('Time [s]');
ylabel('P(X \leq x)');
title('');
grid on;
legend('Mobile IPv6','MoVeNet','MoVeNet (MH)','Location','southeast');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_1_to_1_TCP_MA_to_CN_ecdf.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%%  Handover Delay: UDP CN_MA
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
v = [vecHandoverDelay_Mobileipv6_1_to_1_UDP_CN_to_MA vecHandoverDelay_Movenet_1_to_1_UDP_CN_to_MA vecHandoverDelay_Movenet_multi_1_to_1_UDP_CN_to_MA];
g = [zeros(length(vecHandoverDelay_Mobileipv6_1_to_1_UDP_CN_to_MA),1); ones(length(vecHandoverDelay_Movenet_1_to_1_UDP_CN_to_MA),1); (zeros(length(vecHandoverDelay_Movenet_multi_1_to_1_UDP_CN_to_MA),1)+2);];
boxplot(v,g);
boxplotLabel(3,{'Mobile IPv6','MoVeNet','MoVe-(MH)'}, '');
grid on;
ylim([0.001 100]);
set(gca,'YScale','log');
ylabel('Time [s]');
% xlabel(['Delay TCP VN->CN' char(10) 'MobileIPv6']);
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_1_to_1_UDP_CN_to_MA_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
% ECDF
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
hold on;
h1 = cdfplot(vecHandoverDelay_Mobileipv6_1_to_1_UDP_CN_to_MA);
h2 = cdfplot(vecHandoverDelay_Movenet_1_to_1_UDP_CN_to_MA);
h3 = cdfplot(vecHandoverDelay_Movenet_multi_1_to_1_UDP_CN_to_MA);
hold off;
set(h1, 'Color',[0.8,0.4,0]);
set(h2, 'Color',[0,0.4,0.8]);
set(h3, 'Color',[0.2,0.6,0.4]);
xlabel('Time [s]');
ylabel('P(X \leq x)');
title('');
grid on;
legend('Mobile IPv6','MoVeNet','MoVeNet (MH)','Location','southeast');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_1_to_1_UDP_CN_to_MA_ecdf.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%%  Handover Delay: UDP MA_CN
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
v = [vecHandoverDelay_Mobileipv6_1_to_1_UDP_MA_to_CN vecHandoverDelay_Movenet_1_to_1_UDP_MA_to_CN vecHandoverDelay_Movenet_multi_1_to_1_UDP_MA_to_CN];
g = [zeros(length(vecHandoverDelay_Mobileipv6_1_to_1_UDP_MA_to_CN),1); ones(length(vecHandoverDelay_Movenet_1_to_1_UDP_MA_to_CN),1); (zeros(length(vecHandoverDelay_Movenet_multi_1_to_1_UDP_MA_to_CN),1)+2);];
boxplot(v,g);
boxplotLabel(3,{'Mobile IPv6','MoVeNet','MoVe-(MH)'}, '');
grid on;
ylim([0.001 100]);
set(gca,'YScale','log');
ylabel('Time [s]');
% xlabel(['Delay TCP VN->CN' char(10) 'MobileIPv6']);
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_1_to_1_UDP_MA_to_CN_box.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end
% ECDF
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
hold on;
h1 = cdfplot(vecHandoverDelay_Mobileipv6_1_to_1_UDP_MA_to_CN);
h2 = cdfplot(vecHandoverDelay_Movenet_1_to_1_UDP_MA_to_CN);
h3 = cdfplot(vecHandoverDelay_Movenet_multi_1_to_1_UDP_MA_to_CN);
hold off;
set(h1, 'Color',[0.8,0.4,0]);
set(h2, 'Color',[0,0.4,0.8]);
set(h3, 'Color',[0.2,0.6,0.4]);
xlabel('Time [s]');
ylabel('P(X \leq x)');
title('');
grid on;
legend('Mobile IPv6','MoVeNet','MoVeNet (MH)','Location','southeast');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/ho_1_to_1_UDP_MA_to_CN_ecdf.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%% RTT TCP CN_MA
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
hold on;
h1 = scatter(1:numel(vecRttMean_Mobileipv6_1_to_1_TCP_CN_to_MA),vecRttMean_Mobileipv6_1_to_1_TCP_CN_to_MA,'*');
h2 = scatter(1:numel(vecRttMean_Movenet_1_to_1_TCP_CN_to_MA),vecRttMean_Movenet_1_to_1_TCP_CN_to_MA,'s');
h3 = scatter(1:numel(vecRttMean_Movenet_multi_1_to_1_TCP_CN_to_MA),vecRttMean_Movenet_multi_1_to_1_TCP_CN_to_MA,'o');
legend('Mobile IPv6','MoVeNet','MoVeNet (MH)');
xlabel('Simulation runs');
ylabel('Time [s]');
grid on;
set(h1, 'MarkerEdgeColor',[0.8,0.4,0]);
set(h2, 'MarkerEdgeColor',[0,0.4,0.8]);
set(h3, 'MarkerEdgeColor',[0.2,0.6,0.4]);
ylim([0.1 10]);
set(gca,'YScale','log');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_1_to_1_TCP_CN_to_MA_sca.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%% RTT TCP MA_CN
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
hold on;
h1 = scatter(1:numel(vecRttMean_Mobileipv6_1_to_1_TCP_MA_to_CN),vecRttMean_Mobileipv6_1_to_1_TCP_MA_to_CN,'*');
h2 = scatter(1:numel(vecRttMean_Movenet_1_to_1_TCP_MA_to_CN),vecRttMean_Movenet_1_to_1_TCP_MA_to_CN,'s');
h3 = scatter(1:numel(vecRttMean_Movenet_multi_1_to_1_TCP_MA_to_CN),vecRttMean_Movenet_multi_1_to_1_TCP_MA_to_CN,'o');
legend('Mobile IPv6','MoVeNet','MoVeNet (MH)');
xlabel('Simulation runs');
ylabel('Time [s]');
grid on;
set(h1, 'MarkerEdgeColor',[0.8,0.4,0]);
set(h2, 'MarkerEdgeColor',[0,0.4,0.8]);
set(h3, 'MarkerEdgeColor',[0.2,0.6,0.4]);
ylim([0.1 10]);
set(gca,'YScale','log');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_1_to_1_TCP_MA_to_CN_sca.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%% RTT UDP CN_MA
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
hold on;
h1 = scatter(1:numel(vecRttMean_Mobileipv6_1_to_1_UDP_CN_to_MA),vecRttMean_Mobileipv6_1_to_1_UDP_CN_to_MA,'*');
h2 = scatter(1:numel(vecRttMean_Movenet_1_to_1_UDP_CN_to_MA),vecRttMean_Movenet_1_to_1_UDP_CN_to_MA,'s');
h3 = scatter(1:numel(vecRttMean_Movenet_multi_1_to_1_UDP_CN_to_MA),vecRttMean_Movenet_multi_1_to_1_UDP_CN_to_MA,'o');
legend('Mobile IPv6','MoVeNet','MoVeNet (MH)');
xlabel('Simulation runs');
ylabel('Time [s]');
grid on;
set(h1, 'MarkerEdgeColor',[0.8,0.4,0]);
set(h2, 'MarkerEdgeColor',[0,0.4,0.8]);
set(h3, 'MarkerEdgeColor',[0.2,0.6,0.4]);
% ylim([0.01 1]);
% set(gca,'YScale','log');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_1_to_1_UDP_CN_to_MA_sca.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

%% RTT UDP MA_CN
idx_plot = idx_plot + 1;
figure(idx_plot); clf;
hold on;
h1 = scatter(1:numel(vecRttMean_Mobileipv6_1_to_1_UDP_MA_to_CN),vecRttMean_Mobileipv6_1_to_1_UDP_MA_to_CN,'*');
h2 = scatter(1:numel(vecRttMean_Movenet_1_to_1_UDP_MA_to_CN),vecRttMean_Movenet_1_to_1_UDP_MA_to_CN,'s');
h3 = scatter(1:numel(vecRttMean_Movenet_multi_1_to_1_UDP_MA_to_CN),vecRttMean_Movenet_multi_1_to_1_UDP_MA_to_CN,'o');
legend('Mobile IPv6','MoVeNet','MoVeNet (MH)');
xlabel('Simulation runs');
ylabel('Time [s]');
set(h1, 'MarkerEdgeColor',[0.8,0.4,0]);
set(h2, 'MarkerEdgeColor',[0,0.4,0.8]);
set(h3, 'MarkerEdgeColor',[0.2,0.6,0.4]);
grid on;
% ylim([0.01 1]);
% set(gca,'YScale','log');
if(CREATE_PLOT)
    matlab2tikz([output_folder '/rtt_1_to_1_UDP_MA_to_CN_sca.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
end

clear ax1 ax2 ax3 ax4 ax5 ax6 idx_plot CREATE_PLOT CREATE_TEX PLOT v g CALC communication main_output_folder output_folder h1 h2 h3 h;
%

%%
% ax1 = subplot(1,6,1);
% boxplot(vecHandoverDelay_Mobileipv6_1_to_1_TCP_CN_to_MA);
% grid on;
% ylim(ax1,[0.000001 30]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['Delay TCP CN->VN' char(10) 'MobileIPv6']); 
% ax2 = subplot(1,6,2);
% boxplot(vecHandoverDelay_Movenet_1_to_1_TCP_CN_to_MA);
% grid on;
% ylim(ax2,[0.000001 30]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['Delay TCP CN->VN' char(10) 'MoVeNet']);
% ax3 = subplot(1,6,3);
% boxplot(vecHandoverDelay_Movenet_multi_1_to_1_TCP_CN_to_MA);
% grid on;
% ylim(ax3,[0.000001 30]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['Delay TCP CN->VN' char(10) 'MoVeNet Multi-homing']);
% grid on;
% ax4 = subplot(1,6,4);
% boxplot(vecHandoverDelay_Mobileipv6_1_to_1_TCP_MA_to_CN);
% grid on;
% set(gca,'YScale','log');
% ylim(ax4,[0.000001 30]);
% ylabel('Time [s]');
% xlabel(['Delay TCP VN->CN' char(10) 'MobileIPv6']);
% ax5 = subplot(1,6,5);
% boxplot(vecHandoverDelay_Movenet_1_to_1_TCP_MA_to_CN);
% grid on;
% set(gca,'YScale','log');
% ylim(ax5,[0.000001 30]);
% ylabel('Time [s]');
% xlabel(['Delay TCP VN->CN' char(10) 'MoVeNet']);
% ax6 = subplot(1,6,6);
% boxplot(vecHandoverDelay_Movenet_multi_1_to_1_TCP_MA_to_CN);
% grid on;
% ylim(ax6,[0.000001 30]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['Delay TCP VN->CN' char(10) 'MoVeNet Multi-homing']);
% if(CREATE_PLOT)
%     matlab2tikz([output_folder '/ho_1_to_1_TCP_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
% end
% idx_plot = idx_plot + 1;
% figure(idx_plot); clf;
% ax1 = subplot(1,6,1);
% boxplot(vecHandoverDelay_Mobileipv6_1_to_1_UDP_CN_to_MA);
% grid on;
% ylim(ax1,[0.000001 30]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['Delay UDP CN->VN' char(10) 'MobileIPv6']); 
% ax2 = subplot(1,6,2);
% boxplot(vecHandoverDelay_Movenet_1_to_1_UDP_CN_to_MA);
% grid on;
% set(gca,'YScale','log');
% ylim(ax2,[0.000001 30]);
% ylabel('Time [s]');
% xlabel(['Delay UDP CN->VN' char(10) 'MoVeNet']);
% ax3 = subplot(1,6,3);
% boxplot(vecHandoverDelay_Movenet_multi_1_to_1_UDP_CN_to_MA);
% grid on;
% ylim(ax3,[0.000001 30]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['Delay UDP CN->VN' char(10) 'MoVeNet Multi-homing']);
% ax4 = subplot(1,6,4);
% boxplot(vecHandoverDelay_Mobileipv6_1_to_1_UDP_MA_to_CN);
% grid on;
% ylim(ax4,[0.000001 30]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['Delay UDP VN->CN' char(10) 'MobileIPv6']);
% ax5 = subplot(1,6,5);
% boxplot(vecHandoverDelay_Movenet_1_to_1_UDP_MA_to_CN);
% grid on;
% set(gca,'YScale','log');
% ylim(ax5,[0.000001 30]);
% ylabel('Time [s]');
% xlabel(['Delay UDP VN->CN' char(10) 'MoVeNet']);
% ax6 = subplot(1,6,6);
% boxplot(vecHandoverDelay_Movenet_multi_1_to_1_UDP_MA_to_CN);
% grid on;
% ylim(ax6,[0.000001 30]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['Delay UDP VN->CN' char(10) 'MoVeNet Multi-homing']);
% if(CREATE_PLOT)
%     matlab2tikz([output_folder '/ho_1_to_1_UDP_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
% end

%%
% ax1 = subplot(1,6,1);
% boxplot(vecRttMean_Mobileipv6_1_to_1_TCP_CN_to_MA);
% grid on;
% ylim(ax1,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT TCP CN->VN' char(10) 'MobileIPv6']); 
% ax2 = subplot(1,6,2);
% boxplot(vecRttMean_Movenet_1_to_1_TCP_CN_to_MA);
% grid on;
% ylim(ax2,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT TCP CN->VN' char(10) 'MoVeNet']);
% ax3 = subplot(1,6,3);
% boxplot(vecRttMean_Movenet_multi_1_to_1_TCP_CN_to_MA);
% grid on;
% ylim(ax3,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT TCP CN->VN' char(10) 'MoVeNet Multi-homing']);
% ax4 = subplot(1,6,4);
% boxplot(vecRttMean_Mobileipv6_1_to_1_TCP_MA_to_CN);
% grid on;
% ylim(ax4,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT TCP VN->CN' char(10) 'MobileIPv6']);
% ax5 = subplot(1,6,5);
% boxplot(vecRttMean_Movenet_1_to_1_TCP_MA_to_CN);
% grid on;
% ylim(ax5,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT TCP VN->CN' char(10) 'MoVeNet']);
% ax6 = subplot(1,6,6);
% boxplot(vecRttMean_Movenet_multi_1_to_1_TCP_MA_to_CN);
% grid on;
% ylim(ax6,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT TCP VN->CN' char(10) 'MoVeNet Multi-homing']);
% if(CREATE_PLOT)
%     matlab2tikz([output_folder '/rtt_1_to_1_TCP_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
% end

% idx_plot = idx_plot + 1;
% figure(idx_plot); clf;
% ax1 = subplot(1,6,1);
% boxplot(vecRttMean_Mobileipv6_1_to_1_UDP_CN_to_MA);
% grid on;
% ylim(ax1,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT UDP CN->VN' char(10) 'MobileIPv6']); 
% ax2 = subplot(1,6,2);
% boxplot(vecRttMean_Movenet_1_to_1_UDP_CN_to_MA);
% grid on;
% ylim(ax2,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT UDP CN->VN' char(10) 'MoVeNet']);
% ax3 = subplot(1,6,3);
% boxplot(vecRttMean_Movenet_multi_1_to_1_UDP_CN_to_MA);
% grid on;
% ylim(ax3,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT UDP CN->VN' char(10) 'MoVeNet Multi-homing']);
% ax4 = subplot(1,6,4);
% boxplot(vecRttMean_Mobileipv6_1_to_1_UDP_MA_to_CN);
% grid on;
% ylim(ax4,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT UDP VN->CN' char(10) 'MobileIPv6']);
% ax5 = subplot(1,6,5);
% boxplot(vecRttMean_Movenet_1_to_1_UDP_MA_to_CN);
% grid on;
% ylim(ax5,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT UDP VN->CN' char(10) 'MoVeNet']);
% ax6 = subplot(1,6,6);
% boxplot(vecRttMean_Movenet_multi_1_to_1_UDP_MA_to_CN);
% grid on;
% ylim(ax6,[0.01 10]);
% set(gca,'YScale','log');
% ylabel('Time [s]');
% xlabel(['RTT UDP VN->CN' char(10) 'MoVeNet Multi-homing']);
% if(CREATE_PLOT)
%     matlab2tikz([output_folder '/rtt_1_to_1_UDP_fig.tex'],'width','\figWidth','height','\figHeight','showInfo', false);
% end

