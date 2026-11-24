#include "PerformanceHistory.h"
#include <SD.h>
#include <SPI.h>
#include <time.h>
#include <math.h>

namespace {
constexpr const char* DIR = "/HvacMonitor";
constexpr const char* WEEKLY = "/HvacMonitor/performance_weekly.csv";
struct Mount {
    bool ready;
    Mount() { SD.end(); delay(5); ready = SD.begin(4, SPI, 4000000, "/sd", 5, false); }
    ~Mount() { SD.end(); }
};
struct Aggregate {
    int16_t bin;
    uint8_t mode;
    uint32_t count;
    double sum;
};
bool parseDaily(const String& line, Aggregate& a) {
    int p1=line.indexOf(','), p2=line.indexOf(',',p1+1), p3=line.indexOf(',',p2+1);
    if(p1<0||p2<0||p3<0) return false;
    String mode=line.substring(0,p1);
    if(mode!="HEAT"&&mode!="COOL") return false;
    a.mode=mode=="COOL"; a.bin=line.substring(p1+1,p2).toInt();
    a.count=(uint32_t)strtoul(line.substring(p2+1,p3).c_str(),nullptr,10);
    a.sum=strtod(line.substring(p3+1).c_str(),nullptr);
    return a.count>0 && isfinite(a.sum);
}
}
String PerformanceHistory::iso(uint32_t utc) {
    time_t t=utc; struct tm tmv={}; gmtime_r(&t,&tmv);
    char b[32]; strftime(b,sizeof(b),"%Y-%m-%dT%H:%M:%SZ",&tmv);
    return String(b);
}
String PerformanceHistory::dailyName(uint32_t day) {
    time_t t=weekStart(day); struct tm tmv={}; gmtime_r(&t,&tmv);
    char b[64]; snprintf(b,sizeof(b),"%s/daily_%04d%02d%02d.%u.csv",DIR,
        tmv.tm_year+1900,tmv.tm_mon+1,tmv.tm_mday,
        (unsigned)((day-weekStart(day))/86400U));
    return String(b);
}
void PerformanceHistory::persist() {
    Preferences p; if(!p.begin("perfday",false)) return;
    p.putUInt("day",_day);
    p.putBytes("bins",_bins,sizeof(_bins));
    p.putUChar("count",_count);
    p.end();
}
void PerformanceHistory::restore() {
    Preferences p; if(!p.begin("perfday",true)) return;
    _day=p.getUInt("day",0);
    if(p.getBytesLength("bins")==sizeof(_bins))
        p.getBytes("bins",_bins,sizeof(_bins));
    _count=p.getUChar("count",0);
    if(_count>MAX_BINS) _count=0;
    p.end();
}
void PerformanceHistory::begin() {
    restore();
    _ready=true;
    update((uint32_t)time(nullptr));
}
bool PerformanceHistory::flushDay() {
    if(!_day||!_count) return true;
    Mount m; if(!m.ready) return false;
    String path=dailyName(_day);
    if(SD.exists(path)) return true; // retry after power failure: do not duplicate
    String temp=path+".tmp";
    SD.remove(temp);
    File f=SD.open(temp,FILE_WRITE); if(!f) return false;
    f.println("Mode,OutdoorBinC,EventCount,ScoreSum,AverageScore");
    for(uint8_t i=0;i<_count;i++) {
        const Bin& b=_bins[i];
        if(!b.count) continue;
        f.printf("%s,%d,%lu,%.8f,%.8f\n",b.mode?"COOL":"HEAT",
            (int)b.outdoorBinC,(unsigned long)b.count,b.scoreSum,b.scoreSum/b.count);
    }
    f.flush(); f.close();
    if(!SD.rename(temp,path)) { SD.remove(temp); return false; }
    return true;
}
void PerformanceHistory::update(uint32_t now) {
    if(!_ready||now<1700000000U) return;
    uint32_t today=dayStart(now);
    if(!_day) { _day=today; persist(); }
    if(_day<today) {
        if(!flushDay()) return;
        memset(_bins,0,sizeof(_bins)); _count=0; _day=today; persist();
    }
    // Run catch-up once per week or at startup, not every ten-second sample.
    static uint32_t lastChecked=0;
    uint32_t week=weekStart(today);
    if(lastChecked!=week) { consolidate(week); lastChecked=week; }
}
void PerformanceHistory::record(const LoadEvent& e) {
    if(!_ready||!e.complete||!isfinite(e.avgOutdoorC)||
       !isfinite(e.performanceScore)||e.performanceScore<=0) return;
    uint32_t end=e.start+e.seconds;
    update(end);
    if(_day!=dayStart(end)) return; // failed rollover: do not misattribute
    int16_t bin=(int16_t)(floorf(e.avgOutdoorC/5.0f)*5);
    uint8_t i=0;
    for(;i<_count;i++) if(_bins[i].outdoorBinC==bin && _bins[i].mode==0) break;
    if(i==_count) {
        if(_count>=MAX_BINS) return;
        _bins[i]=Bin{}; _bins[i].outdoorBinC=bin; ++_count;
    }
    _bins[i].count++;
    _bins[i].scoreSum+=e.performanceScore;
    persist();
}
void PerformanceHistory::consolidate(uint32_t currentWeek) {
    Mount m; if(!m.ready) return;
    File dir=SD.open(DIR); if(!dir||!dir.isDirectory()) return;
    // Collect distinct completed week identifiers from daily filenames.
    uint32_t weeks[110]={}; size_t n=0;
    File f=dir.openNextFile();
    while(f) {
        String name=f.name(); name=name.substring(name.lastIndexOf('/')+1);
        if(!f.isDirectory()&&name.startsWith("daily_")&&name.endsWith(".csv")&&name.length()==20) {
            String date=name.substring(6,14);
            struct tm tmv={}; if(strptime(date.c_str(),"%Y%m%d",&tmv)) {
                tmv.tm_isdst=0; uint32_t w=(uint32_t)timegm(&tmv);
                if(w<currentWeek&&weekStart(w)==w) {
                    bool seen=false; for(size_t i=0;i<n;i++) if(weeks[i]==w) seen=true;
                    if(!seen&&n<110) weeks[n++]=w;
                }
            }
        }
        f.close(); f=dir.openNextFile();
    }
    dir.close();
    for(size_t k=0;k<n;k++) {
        uint32_t w=weeks[k];
        // If any row already exists for this week, assume it was committed.
        // A commit marker below is the authoritative completion indicator.
        bool committed=false;
        if(SD.exists(WEEKLY)) {
            File check=SD.open(WEEKLY,FILE_READ);
            String key=iso(w);
            while(check&&check.available()) {
                String line=check.readStringUntil('\n');
                if(line.startsWith(key+",COMMIT,")) { committed=true; break; }
            }
            if(check) check.close();
        }
        if(!committed) {
            // Remove an interrupted append for this week before retrying.
            // No daily files are deleted until a COMMIT marker is durable.
            if(SD.exists(WEEKLY)) {
                File old=SD.open(WEEKLY,FILE_READ);
                const char* clean="/HvacMonitor/.weekly.clean";
                SD.remove(clean);
                File out=SD.open(clean,FILE_WRITE);
                if(!old||!out) { if(old)old.close(); if(out)out.close(); continue; }
                String key=iso(w);
                while(old.available()) {
                    String line=old.readStringUntil('\n');
                    if(!line.startsWith(key+",")) out.println(line);
                }
                out.flush(); old.close(); out.close();
                if(!SD.remove(WEEKLY) || !SD.rename(clean,WEEKLY)) continue;
            }
            Aggregate bins[64]={}; size_t count=0;
            for(uint8_t d=0;d<7;d++) {
                String path=dailyName(w+d*86400U);
                File daily=SD.open(path,FILE_READ);
                if(!daily) continue;
                daily.readStringUntil('\n');
                while(daily.available()) {
                    Aggregate a={}; if(!parseDaily(daily.readStringUntil('\n'),a)) continue;
                    size_t j=0;
                    for(;j<count;j++) if(bins[j].bin==a.bin&&bins[j].mode==a.mode) break;
                    if(j==count) {
                        if(count>=64) continue;
                        bins[count++]=a;
                    } else { bins[j].count+=a.count; bins[j].sum+=a.sum; }
                }
                daily.close();
            }
            if(!count) continue;
            // Write complete week to a staging file first, then append it.
            // COMMIT makes partial append detectable on the next boot.
            const char* stage="/HvacMonitor/.weekly.tmp";
            SD.remove(stage);
            File staged=SD.open(stage,FILE_WRITE); if(!staged) continue;
            String key=iso(w);
            for(size_t j=0;j<count;j++) {
                Aggregate& a=bins[j];
                staged.printf("%s,%s,%d,%lu,%.8f,%.8f\n",key,
                    a.mode?"COOL":"HEAT",(int)a.bin,(unsigned long)a.count,
                    a.sum,a.sum/a.count);
            }
            staged.printf("%s,COMMIT,0,0,0,0\n",key.c_str());
            staged.flush(); staged.close();
            File src=SD.open(stage,FILE_READ);
            bool newFile=!SD.exists(WEEKLY);
            File dst=SD.open(WEEKLY,FILE_APPEND);
            if(!src||!dst) { if(src)src.close();if(dst)dst.close();continue; }
            if(newFile) dst.println("WeekStartUTC,Mode,OutdoorBinC,EventCount,ScoreSum,AverageScore");
            while(src.available()) dst.write((uint8_t)src.read());
            dst.flush(); src.close(); dst.close(); SD.remove(stage);
            // Verify the commit marker before deleting the daily source files.
            File verify=SD.open(WEEKLY,FILE_READ); bool ok=false;
            while(verify&&verify.available())
                if(verify.readStringUntil('\n').startsWith(key+",COMMIT,")) ok=true;
            if(verify)verify.close();
            if(!ok) continue;
        }
        for(uint8_t d=0;d<7;d++) SD.remove(dailyName(w+d*86400U));
    }
}
