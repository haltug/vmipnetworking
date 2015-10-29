function [handle] = boxplotLabel(dSize,XTickLabel,XLabel)
%BOXPLOTLABEL Summary of this function goes here
%   Detailed explanation goes here
txt    = findall(gcf,'type','text');
delete(txt)
for i=1:dSize                % number of box plot bars in figure
    text('Interpreter','latex',...
         'String',XTickLabel{i},...
         'Units','normalized',...
         'VerticalAlignment','top',...
         'HorizontalAlignment','center',...
         'Position',[1/(dSize*2)*(1+2*(i-1)),-0.01],...x/y position
         'EdgeColor','none')
end
text('Interpreter','latex',...
     'String',XLabel,...
     'Units','normalized',...
     'VerticalAlignment','top',...
     'HorizontalAlignment','center',...
     'Position',[0.5,-0.07],...
     'EdgeColor','none')
handle = gcf;
end
