#!/usr/bin/env python3
"""Leakage-free fitness selection for QGenClass native .train/.test data files."""
import argparse, csv, math, random, re, statistics, subprocess, sys
from collections import Counter, defaultdict
from pathlib import Path

CANDIDATES = [
    ('mean', 'mean', {}),
    ('minority_f1', 'minority_f1', {}),
    ('fbeta_2', 'minority_fbeta', {'fbeta-beta': '2'}),
    ('fbeta_3', 'minority_fbeta', {'fbeta-beta': '3'}),
    ('hybrid_090', 'hybrid_sampling', {'sampling-lambda':'0.90','sampling-ratio':'3','sampling-interval':'20'}),
    ('constrained_p5', 'constrained_recall', {'fpr-limit':'0.10','fpr-penalty':'5'}),
    ('constrained_p10', 'constrained_recall', {'fpr-limit':'0.10','fpr-penalty':'10'}),
    ('constrained_p20', 'constrained_recall', {'fpr-limit':'0.10','fpr-penalty':'20'}),
]

def read_data(path):
    toks = Path(path).read_text().split()
    if len(toks)<2: raise ValueError(f'Invalid data header: {path}')
    d, n = int(toks[0]), int(toks[1])
    if d<1 or n<1 or len(toks)!=2+n*(d+1):
        raise ValueError(f'Invalid data dimensions in {path}: d={d}, n={n}, tokens={len(toks)}')
    rows = [toks[2+i*(d+1):2+(i+1)*(d+1)] for i in range(n)]
    return d,rows

def write_data(path,d,rows):
    Path(path).write_text(f'{d}\n{len(rows)}\n'+''.join(' '.join(row)+'\n' for row in rows))

def split_rows(rows, frac, seed):
    groups=defaultdict(list)
    for row in rows: groups[row[-1]].append(row)
    rng=random.Random(seed)
    train,valid=[],[]
    for label,group in sorted(groups.items()):
        if len(group)<2: raise ValueError(f'Class {label} has fewer than 2 training examples')
        group=group.copy(); rng.shuffle(group)
        k=max(1,min(len(group)-1,round(len(group)*frac)))
        valid.extend(group[:k]);train.extend(group[k:])
    rng.shuffle(train);rng.shuffle(valid)
    return train,valid

def parse_confusion(log):
    lines=log.splitlines()
    found=[]
    for i,line in enumerate(lines):
        m=re.search(r'CONFUSION MATRIX\s*\*\*.*?Number of classes:\s*(\d+)',line,re.I)
        if not m: continue
        nc=int(m.group(1)); mat=[]
        for nextline in lines[i+1:i+1+nc]:
            vals=nextline.split()
            if len(vals)!=nc: break
            try: mat.append([int(v) for v in vals])
            except ValueError: break
        if len(mat)==nc: found.append(mat)
    if not found: raise ValueError('No valid CONFUSION MATRIX found in QGenClass output')
    return found[-1]

def metrics(mat):
    if len(mat)!=2: raise ValueError('This experiment requires binary classification')
    counts=[sum(row) for row in mat]
    if min(counts)==0: raise ValueError('Missing class in confusion matrix')
    minority=counts.index(min(counts))
    if counts[0]==counts[1]: raise ValueError('Validation classes have equal counts; minority ambiguous')
    tp=mat[minority][minority]; fn=counts[minority]-tp
    fp=sum(mat[r][minority] for r in range(2) if r!=minority)
    tn=sum(mat[r][r] for r in range(2) if r!=minority)
    precision=tp/(tp+fp) if tp+fp else 0.0
    recall=tp/(tp+fn) if tp+fn else 0.0
    f2=5*tp/(5*tp+4*fn+fp) if 5*tp+4*fn+fp else 0.0
    f1=2*tp/(2*tp+fn+fp) if 2*tp+fn+fp else 0.0
    balanced=(recall+tn/(tn+fp))/2
    return dict(tp=tp,fp=fp,fn=fn,tn=tn,minority_precision=precision,
                minority_recall=recall,minority_f1=f1,minority_f2=f2,balanced_accuracy=balanced, fpr=fp/(fp+tn))

def execute(args,train,test,fitness,extras,seed,log_path):
    cmd=[str(args.script),'--exe',str(args.exe),'--train',str(train),'--test',str(test),
         '--fitness',fitness,'--runs','1','--seed',str(seed),
         '--generations',str(args.generations),'--chromosomes',str(args.chromosomes),
         '--smote','no','--results-dir',str(log_path.parent/'raw')]
    for key,val in extras.items(): cmd.extend(['--'+key,val])
    if args.extra:
        for pair in args.extra:
            key,sep,val=pair.partition('=')
            if not sep or not key.startswith('--'): raise ValueError('--extra must be --name=value')
            if key in ('--test','--train','--dataset','--data-dir','--fitness','--seed','--runs','--smote'):
                raise ValueError(f'Cannot override protected argument {key}')
            cmd.extend([key,val])
    proc=subprocess.run(cmd,cwd=args.project,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    log_path.write_text('COMMAND: '+' '.join(cmd)+'\n\n'+proc.stdout)
    if proc.returncode: raise RuntimeError(f'QGenClass exit {proc.returncode}; inspect {log_path}')
    return metrics(parse_confusion(proc.stdout))

def save_csv(path,rows):
    if not rows: return
    with Path(path).open('w',newline='') as fp:
        writer=csv.DictWriter(fp,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--project',type=Path,default=Path.cwd())
    ap.add_argument('--data-dir',type=Path,required=True)
    ap.add_argument('--dataset',required=True,help='Basename without .train/.test')
    ap.add_argument('--output',type=Path,default=Path('results/fitness_validation'))
    ap.add_argument('--validation-fraction',type=float,default=.20)
    ap.add_argument('--splits',type=int,default=3)
    ap.add_argument('--seeds-per-split',type=int,default=1)
    ap.add_argument('--seed',type=int,default=1)
    ap.add_argument('--generations',type=int,default=500)
    ap.add_argument('--chromosomes',type=int,default=500)
    ap.add_argument('--final-runs',type=int,default=10)
    ap.add_argument('--criterion',choices=['minority_f2','minority_f1','balanced_accuracy','recall_at_fpr'],default='recall_at_fpr')
    ap.add_argument('--fpr-limit',type=float,default=0.10,help='Validation FPR ceiling for recall_at_fpr')
    ap.add_argument('--fpr-policy',choices=['strict_fpr','mean_fpr'],default='strict_fpr',
                    help='strict_fpr: every validation run must meet limit; mean_fpr: mean validation FPR must meet limit')
    ap.add_argument('--extra',action='append',help='Optional additional runmodel option as --extra=--name=value')
    ap.add_argument('--skip-final',action='store_true',help='Tune only; do not access original test set')
    ap.add_argument('--resume-from',type=Path,help='Read existing validation_runs.csv instead of rerunning validation')
    args=ap.parse_args()
    args.project=args.project.resolve();args.data_dir=args.data_dir.resolve();args.output=args.output.resolve()
    args.script=args.project/'runmodel.sh';args.exe=args.project/'QGenClass'
    if not args.script.is_file() or not args.exe.is_file(): ap.error('Expected runmodel.sh and QGenClass executable inside --project')
    if not 0<args.validation_fraction<.5 or args.splits<1 or args.seeds_per_split<1 or args.final_runs<1: ap.error('Invalid split/run settings')
    if not 0<=args.fpr_limit<=1: ap.error('--fpr-limit must be in [0,1]')
    train_file=args.data_dir/(args.dataset+'.train')
    test_file=args.data_dir/(args.dataset+'.test')
    args.output.mkdir(parents=True,exist_ok=True)
    all_results=[]
    if args.resume_from:
        source=args.resume_from.resolve()
        if not source.is_file(): ap.error(f'Validation CSV not found: {source}')
        with source.open(newline='') as fp:
            reader=csv.DictReader(fp)
            required={'candidate','split','seed','tp','fp','fn','tn','minority_precision',
                      'minority_recall','minority_f1','minority_f2','balanced_accuracy','fpr'}
            if not reader.fieldnames or not required.issubset(reader.fieldnames):
                ap.error('Validation CSV has missing required columns')
            for row in reader:
                parsed={k:(int(v) if k in ('split','seed','tp','fp','fn','tn') else
                           float(v) if k not in ('candidate',) else v) for k,v in row.items()}
                all_results.append(parsed)
        expected={label for label,_,_ in CANDIDATES}
        present={r['candidate'] for r in all_results}
        if present!=expected:
            ap.error(f'Candidate mismatch: missing={sorted(expected-present)}, extra={sorted(present-expected)}')
        keys=[(r['candidate'],r['split'],r['seed']) for r in all_results]
        if len(keys)!=len(set(keys)):
            ap.error('Duplicate candidate/split/seed rows in validation CSV')
        per_candidate=Counter(r['candidate'] for r in all_results)
        if len(set(per_candidate.values()))!=1:
            ap.error(f'Unequal validation runs across candidates: {dict(per_candidate)}')
        # Keep a local copy of the input for reproducibility.
        destination=args.output/'validation_runs.csv'
        if source!=destination:
            save_csv(destination,all_results)
        print(f'Resuming from {source} ({len(all_results)} validation rows); no validation runs repeated.',flush=True)
    else:
        d,rows=read_data(train_file)
        if len(set(r[-1] for r in rows))!=2: ap.error('Only binary classification is supported')
        print('Training class counts:',dict(Counter(r[-1] for r in rows)),flush=True)
        val_dir=args.output/'validation';val_dir.mkdir(exist_ok=True)
        # Identical splits and random seeds across candidate methods.
        for s in range(args.splits):
            train,valid=split_rows(rows,args.validation_fraction,args.seed+1009*s)
            tr=val_dir/f'split_{s+1}.train';va=val_dir/f'split_{s+1}.test'
            write_data(tr,d,train);write_data(va,d,valid)
            print(f'Split {s+1}/{args.splits}: train={len(train)}, validation={len(valid)}',flush=True)
            for run in range(args.seeds_per_split):
                seed=args.seed+run
                for label,fitness,extras in CANDIDATES:
                    log=val_dir/f'{label}_split{s+1}_seed{seed}.log'
                    print(f'  {label}, seed={seed} ...',flush=True)
                    result=execute(args,tr,va,fitness,extras,seed,log)
                    all_results.append(dict(candidate=label,split=s+1,seed=seed,**result))
                    save_csv(args.output/'validation_runs.csv',all_results)
                    print(f'    F2={result["minority_f2"]:.4f}, P={result["minority_precision"]:.4f}, R={result["minority_recall"]:.4f}, FP={result["fp"]}',flush=True)
    rankings=[]
    for label,_,_ in CANDIDATES:
        items=[r for r in all_results if r['candidate']==label]
        rankings.append(dict(candidate=label,mean_score=statistics.mean(r['minority_recall' if args.criterion=='recall_at_fpr' else args.criterion] for r in items),
                             std_score=statistics.stdev(r['minority_recall' if args.criterion=='recall_at_fpr' else args.criterion] for r in items) if len(items)>1 else 0,
                             mean_precision=statistics.mean(r['minority_precision'] for r in items),
                             mean_recall=statistics.mean(r['minority_recall'] for r in items),
                             mean_fp=statistics.mean(r['fp'] for r in items),
                             mean_fpr=statistics.mean(r['fpr'] for r in items),
                             max_fpr=max(r['fpr'] for r in items)))
    if args.criterion=='recall_at_fpr':
        fpr_key='max_fpr' if args.fpr_policy=='strict_fpr' else 'mean_fpr'
        for row in rankings:
            row['feasible']=int(row[fpr_key] <= args.fpr_limit + 1e-12)
        # Feasible methods first, then highest mean minority recall.
        # If none is feasible, minimize constraint violation first.
        if any(row['feasible'] for row in rankings):
            rankings.sort(key=lambda row:(-row['feasible'],
                         -row['mean_recall'] if row['feasible'] else row[fpr_key],
                         row['mean_fpr'],row['candidate']))
        else:
            rankings.sort(key=lambda row:(row[fpr_key],-row['mean_recall'],row['candidate']))
    else:
        rankings.sort(key=lambda row:(-row['mean_score'],row['mean_fp'],row['candidate']))
    save_csv(args.output/'validation_ranking.csv',rankings)
    best=rankings[0]['candidate'];candidate=next(c for c in CANDIDATES if c[0]==best)
    print('\nValidation ranking:',flush=True)
    for item in rankings:
        feasible=item.get('feasible','n/a')
        print(f'  {item["candidate"]:17s} score={item["mean_score"]:.4f} +/- {item["std_score"]:.4f} meanFPR={item["mean_fpr"]:.4f} maxFPR={item["max_fpr"]:.4f} feasible={feasible}',flush=True)
    print(f'\nSelected on validation ONLY: {best}',flush=True)
    (args.output/'selected.txt').write_text(f'criterion={args.criterion}\nfpr_policy={args.fpr_policy}\nfpr_limit={args.fpr_limit}\nselected={best}\nfitness={candidate[1]}\nextras={candidate[2]}\n')
    if args.skip_final:
        print('Final test skipped (--skip-final).',flush=True);return
    # Test data is opened only after all tuning decisions are finalized.
    if not test_file.is_file(): raise FileNotFoundError(test_file)
    final_dir=args.output/'final';final_dir.mkdir(exist_ok=True)
    existing_final=args.output/'final_test_runs.csv'
    if existing_final.is_file():
        with existing_final.open(newline='') as fp:
            existing_rows=list(csv.DictReader(fp))
        existing_names={r.get('candidate') for r in existing_rows}
        if existing_names and existing_names != {best}:
            import shutil
            backup=args.output/('final_test_runs_'+str(next(iter(existing_names)))+'.csv')
            if backup.exists():
                raise RuntimeError(f'Backup already exists: {backup}; move it before rerunning')
            shutil.copy2(existing_final,backup)
            print(f'Preserved previous final test results: {backup}',flush=True)
    final=[]
    for run in range(args.final_runs):
        seed=args.seed+run
        log=final_dir/f'{best}_seed{seed}.log'
        print(f'Final test {run+1}/{args.final_runs}: {best}, seed={seed}',flush=True)
        result=execute(args,train_file,test_file,candidate[1],candidate[2],seed,log)
        final.append(dict(candidate=best,seed=seed,**result))
        save_csv(args.output/'final_test_runs.csv',final)
    print('\nFINAL TEST:',flush=True)
    for metric in ['minority_precision','minority_recall','minority_f1','minority_f2','balanced_accuracy','fp','fn']:
        vals=[r[metric] for r in final]
        print(f'  {metric:20s} {statistics.mean(vals):.5f} +/- {(statistics.stdev(vals) if len(vals)>1 else 0):.5f}',flush=True)
    print('Results:',args.output,flush=True)

if __name__=='__main__':
    try: main()
    except (ValueError,RuntimeError,FileNotFoundError) as exc:
        print('ERROR:',exc,file=sys.stderr);sys.exit(1)
