#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main()
{double ozone_hazard_index,nitrogen_oxide_hazard_index,carbon_monoxide_hazard_index;
printf("ozone hazard index \n");
scanf("%lf",&ozone_hazard_index);
printf("nitrogen_oxide_hazard_index \n");
scanf("%lf",&nitrogen_oxide_hazard_index);
printf("carbon_monoxide_hazard_index \n");
scanf("%lf",&carbon_monoxide_hazard_index );
if(ozone_hazard_index > 100 && ozone_hazard_index < 200){printf("unhealthful");}
else if (ozone_hazard_index>200&&ozone_hazard_index < 275){printf("first-stage smog alert");}
else if  (ozone_hazard_index > 275){printf("second-stage smog alert");}
else if(nitrogen_oxide_hazard_index>100&&nitrogen_oxide_hazard_index<200){printf("unhealthful");}
else if (nitrogen_oxide_hazard_index>200&&nitrogen_oxide_hazard_index<275){printf("first-stage smog alert");}
else if (nitrogen_oxide_hazard_index>275){printf("second-stage smog alert");}
else if(carbon_monoxide_hazard_index>100&&carbon_monoxide_hazard_index<200){printf("unhealthful");}
else if (carbon_monoxide_hazard_index>200&&carbon_monoxide_hazard_index<275){printf("first-stage smog alert");}
 else if (carbon_monoxide_hazard_index>275){printf("second-stage smog alert");}
    else {printf("healthy");}

    return 0;
}
