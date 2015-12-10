function plot_ropdata( filename, save )
    M=csvread(filename);
    t=M(:,1);
    x_1=M(:,2);
    x_2=M(:,3);
    x_3=M(:,4);
    x_4=M(:,5);
    figure;
    hold on;
    plot(t,x_1);
    plot(t,x_2);
    plot(t,x_3);
    plot(t,x_4);
    title(filename);
    xlabel('time');
    ylabel('counts');
    legend('icache fetch','branch taken','return taken','branch mispredict');
    if save
        print('-dpng',strcat(filename,'.png'));
    end
end

