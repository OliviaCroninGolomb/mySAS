import pandas as pd
import glob
import h5py
import os

dock_path = "C:/Users/ocron/OneDrive - Massachusetts Institute of Technology/Documents/Classes/Summer_2025/OceanOptics/Project/mySAS_DataPackage/pysasData/"
oneb_files = []

for file in glob.glob(f"{dock_path}Raw/*_L1BQC.hdf"):
    oneb_files.append(file)

for file in oneb_files:
    with h5py.File(file, 'r') as f:
        es_df1 = pd.DataFrame(f['IRRADIANCE']['ES'][()])
        li_df1 = pd.DataFrame(f['RADIANCE']['LI'][()])
        lt_df1 = pd.DataFrame(f['RADIANCE']['LT'][()])
        
        if file == oneb_files[0]:
            es_df = pd.DataFrame(f['IRRADIANCE']['ES'][()])
            li_df = pd.DataFrame(f['RADIANCE']['LI'][()])
            lt_df = pd.DataFrame(f['RADIANCE']['LT'][()])
        else:
            es_df = pd.concat([es_df, es_df1], axis = 0)
            li_df = pd.concat([li_df, li_df1], axis = 0)
            lt_df = pd.concat([lt_df, lt_df1], axis = 0)

es_df.to_csv(os.path.join(dock_path, "Processed/L1BQC/pySAS_Es_L1BQC.csv"), index=False)
li_df.to_csv(os.path.join(dock_path, "Processed/L1BQC/pySAS_Li_L1BQC.csv"), index=False)
lt_df.to_csv(os.path.join(dock_path, "Processed/L1BQC/pySAS_Lt_L1BQC.csv"), index=False)
