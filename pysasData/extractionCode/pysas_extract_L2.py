import pandas as pd
import glob
import h5py
import os

dock_path = "C:/Users/ocron/OneDrive - Massachusetts Institute of Technology/Documents/Classes/Summer_2025/OceanOptics/Project/mySAS_DataPackage/pysasData/"
oneb_files = []

for file in glob.glob(f"{dock_path}Raw/*_L2.hdf"):
    oneb_files.append(file)

for file in oneb_files:
    with h5py.File(file, 'r') as f:
        es_df1 = pd.DataFrame(f['IRRADIANCE']['ES_HYPER'][()])
        li_df1 = pd.DataFrame(f['RADIANCE']['LI_HYPER'][()])
        lt_df1 = pd.DataFrame(f['RADIANCE']['LT_HYPER'][()])

        es_sd_df1 = pd.DataFrame(f['IRRADIANCE']['ES_HYPER_sd'][()])
        li_sd_df1 = pd.DataFrame(f['RADIANCE']['LI_HYPER_sd'][()])
        lt_sd_df1 = pd.DataFrame(f['RADIANCE']['LT_HYPER_sd'][()])

        es_unc_df1 = pd.DataFrame(f['IRRADIANCE']['ES_HYPER_unc'][()])
        li_unc_df1 = pd.DataFrame(f['RADIANCE']['LI_HYPER_unc'][()])
        lt_unc_df1 = pd.DataFrame(f['RADIANCE']['LT_HYPER_unc'][()])
        
        if file == oneb_files[0]:
            es_df = pd.DataFrame(f['IRRADIANCE']['ES_HYPER'][()])
            li_df = pd.DataFrame(f['RADIANCE']['LI_HYPER'][()])
            lt_df = pd.DataFrame(f['RADIANCE']['LT_HYPER'][()])

            es_sd_df = pd.DataFrame(f['IRRADIANCE']['ES_HYPER_sd'][()])
            li_sd_df = pd.DataFrame(f['RADIANCE']['LI_HYPER_sd'][()])
            lt_sd_df = pd.DataFrame(f['RADIANCE']['LT_HYPER_sd'][()])
    
            es_unc_df = pd.DataFrame(f['IRRADIANCE']['ES_HYPER_unc'][()])
            li_unc_df = pd.DataFrame(f['RADIANCE']['LI_HYPER_unc'][()])
            lt_unc_df = pd.DataFrame(f['RADIANCE']['LT_HYPER_unc'][()])
        
        else:
            es_df = pd.concat([es_df, es_df1], axis = 0)
            li_df = pd.concat([li_df, li_df1], axis = 0)
            lt_df = pd.concat([lt_df, lt_df1], axis = 0)

            es_sd_df = pd.concat([es_sd_df, es_sd_df1], axis = 0)
            li_sd_df = pd.concat([li_sd_df, li_sd_df1], axis = 0)
            lt_sd_df = pd.concat([lt_sd_df, lt_sd_df1], axis = 0)

            es_unc_df = pd.concat([es_unc_df, es_unc_df1], axis = 0)
            li_unc_df = pd.concat([li_unc_df, li_unc_df1], axis = 0)
            lt_unc_df = pd.concat([lt_unc_df, lt_unc_df1], axis = 0)

es_df.to_csv(os.path.join(dock_path, "Processed/L2/pySAS_Es_L2.csv"), index=False)
li_df.to_csv(os.path.join(dock_path, "Processed/L2/pySAS_Li_L2.csv"), index=False)
lt_df.to_csv(os.path.join(dock_path, "Processed/L2/pySAS_Lt_L2.csv"), index=False)

es_sd_df.to_csv(os.path.join(dock_path, "Processed/L2/pySAS_Es_sd_L2.csv"), index=False)
li_sd_df.to_csv(os.path.join(dock_path, "Processed/L2/pySAS_Li_sd_L2.csv"), index=False)
lt_sd_df.to_csv(os.path.join(dock_path, "Processed/L2/pySAS_Lt_sd_L2.csv"), index=False)

es_unc_df.to_csv(os.path.join(dock_path, "Processed/L2/pySAS_Es_unc_L2.csv"), index=False)
li_unc_df.to_csv(os.path.join(dock_path, "Processed/L2/pySAS_Li_unc_L2.csv"), index=False)
lt_unc_df.to_csv(os.path.join(dock_path, "Processed/L2/pySAS_Lt_unc_L2.csv"), index=False)
