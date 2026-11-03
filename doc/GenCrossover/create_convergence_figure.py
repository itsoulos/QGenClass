#!/usr/bin/env python3
"""Create an eight-panel publication figure from QGenClass convergence TSV files."""
import csv, glob, os, re, tarfile
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from reportlab.pdfgen import canvas
from reportlab.lib.utils import ImageReader

ROOT='/mnt/data'
ARCHIVE=os.path.join(ROOT,'convergence_all_datasets.tar.gz')
OUT=os.path.join(ROOT,'convergence_4datasets_8panels.pdf')
PNG=os.path.join(ROOT,'convergence_4datasets_8panels.png')
AGG=os.path.join(ROOT,'convergence_4datasets_aggregates.csv')
DATASETS=[('dermatology_1','Dermatology'),('ionosphere_1','Ionosphere'),('spambase_1','Spambase'),('vehicle_1','Vehicle')]
METHODS=['standard','targeted','targetedWorst']
COLORS={'standard':'#2864ad','targeted':'#c44536','targetedWorst':'#27815b'}
LABELS={'standard':'Standard','targeted':'Targeted','targetedWorst':'TargetedWorst'}

records={}
with tarfile.open(ARCHIVE,'r:gz') as tar:
    for m in tar.getmembers():
        if not m.isfile() or not m.name.endswith('_convergence.tsv'): continue
        mt=re.match(r'results/(.+?)_standard_class_(standard|targeted|targetedWorst)_seed(\d+)_convergence\.tsv$',m.name)
        if not mt: continue
        dataset,method,seed=mt.groups()
        lines=tar.extractfile(m).read().decode().splitlines()
        rows=list(csv.DictReader(lines,delimiter='\t'))
        x=np.array([float(r['fitness_evals']) for r in rows]); f=np.array([float(r['best_fitness']) for r in rows]); d=np.array([float(r['diversity']) if r['diversity']!='NA' else np.nan for r in rows])
        assert np.all(np.diff(x)>0),m.name
        records[(dataset,method,int(seed))]=(x,f,d)
for dataset,_ in DATASETS:
    for method in METHODS:
        assert sorted(k[2] for k in records if k[:2]==(dataset,method))==list(range(1,31)),(dataset,method)

# Fitness is a best-so-far step function; diversity is interpolated only between valid observations.
GRID=np.arange(1000,100001,1000)
def aggregate(dataset,method):
    F=[]; D=[]
    for seed in range(1,31):
        x,f,d=records[(dataset,method,seed)]
        ix=np.searchsorted(x,GRID,side='right')-1
        F.append(np.where(ix>=0,f[np.maximum(ix,0)],np.nan))
        good=np.isfinite(d)
        xd=x[good]; dd=d[good]
        D.append(np.interp(GRID,xd,dd,left=np.nan,right=np.nan))
    F=np.array(F);D=np.array(D)
    nf=np.sum(np.isfinite(F),axis=0);nd=np.sum(np.isfinite(D),axis=0)
    return dict(f=np.nanmean(F,axis=0),fs=np.nanstd(F,axis=0,ddof=1),d=np.nanmean(D,axis=0),ds=np.nanstd(D,axis=0,ddof=1),nf=nf,nd=nd)
A={(dataset,method):aggregate(dataset,method) for dataset,_ in DATASETS for method in METHODS}
# Strict common-support mask: 30 valid diversity values for ALL methods at each x.
MASK={dataset:np.logical_and.reduce([A[(dataset,m)]['nd']==30 for m in METHODS]) for dataset,_ in DATASETS}

with open(AGG,'w',newline='') as o:
    w=csv.writer(o);w.writerow(['dataset','method','fitness_evaluations','mean_fitness','sd_fitness','n_fitness','mean_diversity','sd_diversity','n_diversity','common_full_support'])
    for dataset,_ in DATASETS:
        for method in METHODS:
            a=A[(dataset,method)]
            for i,x in enumerate(GRID):
                w.writerow([dataset,method,int(x),a['f'][i],a['fs'][i],int(a['nf'][i]),a['d'][i],a['ds'][i],int(a['nd'][i]),int(MASK[dataset][i])])

plt.rcParams.update({'font.family':'DejaVu Sans','font.size':9,'axes.titlesize':11,'axes.labelsize':9,'legend.fontsize':8,'pdf.fonttype':42})
fig,axes=plt.subplots(4,2,figsize=(12.4,15.5),constrained_layout=True)
for i,(dataset,label) in enumerate(DATASETS):
    axf,axd=axes[i]
    for method in METHODS:
        a=A[(dataset,method)];color=COLORS[method]
        axf.fill_between(GRID,a['f']-a['fs'],a['f']+a['fs'],color=color,alpha=.12,lw=0)
        axf.plot(GRID,a['f'],color=color,lw=1.8,label=LABELS[method])
        mask=MASK[dataset]
        axd.fill_between(GRID,a['d']-a['ds'],a['d']+a['ds'],where=mask,color=color,alpha=.12,lw=0)
        axd.plot(GRID,np.where(mask,a['d'],np.nan),color=color,lw=1.8,label=LABELS[method])
    for ax in (axf,axd):
        ax.grid(alpha=.23);ax.set_xlim(0,100000);ax.tick_params(axis='x',labelsize=8)
        ax.ticklabel_format(axis='x',style='plain');ax.set_xlabel('Fitness evaluations')
    axf.set_title(f'{label} — convergence',fontweight='semibold');axf.set_ylabel('Best-so-far fitness')
    axd.set_title(f'{label} — genotypic diversity',fontweight='semibold');axd.set_ylabel('Normalized Hamming distance');axd.set_ylim(0,1)
    axd.text(.98,.04,f'30/30 runs through {GRID[MASK[dataset]][-1]:,} evals',ha='right',va='bottom',transform=axd.transAxes,fontsize=7.5,color='#444444')
    if i==0:
        axf.legend(loc='lower right',frameon=True);axd.legend(loc='upper right',frameon=True)
fig.suptitle('Crossover convergence and population diversity across four datasets',fontsize=15,fontweight='semibold')
fig.savefig(PNG,dpi=180,facecolor='white')
plt.close(fig)
# ReportLab PDF with high-resolution raster figure, suitable for visual review.
from PIL import Image
im=Image.open(PNG); W,H=im.size
pw,ph=595.3,744.1
c=canvas.Canvas(OUT,pagesize=(pw,ph))
margin=12
scale=min((pw-2*margin)/W,(ph-2*margin)/H)
w,h=W*scale,H*scale
c.drawImage(ImageReader(im),(pw-w)/2,(ph-h)/2,width=w,height=h)
c.save()
print('Created:',OUT,PNG,AGG)
for dataset,_ in DATASETS: print(dataset,'common diversity support ends at',GRID[MASK[dataset]][-1], 'evaluations')
