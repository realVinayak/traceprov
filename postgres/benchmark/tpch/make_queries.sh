

for q in `seq 1 22`; do DSS_QUERY=postgres_queries/base/ ./qgen -s 10 -r 1755709829 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_1/base/$q.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/provtrace/ ./qgen -s 10 -r 1755709829 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_1/provtrace/$q.traceprov.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709829 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_1/validate/$q.validate.sql; done

DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709829 11.a >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_1/validate/11.a.validate.sql
DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709829 11.b >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_1/validate/11.b.validate.sql

##########################################################################################################################################################################################

for q in `seq 1 22`; do DSS_QUERY=postgres_queries/base/ ./qgen -s 10 -r 1755709841 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_2/base/$q.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/provtrace/ ./qgen -s 10 -r 1755709841 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_2/provtrace/$q.traceprov.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709841 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_2/validate/$q.validate.sql; done

DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709841 11.a >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_2/validate/11.a.validate.sql
DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709841 11.b >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_2/validate/11.b.validate.sql

##########################################################################################################################################################################################

for q in `seq 1 22`; do DSS_QUERY=postgres_queries/base/ ./qgen -s 10 -r 1755709849 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_3/base/$q.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/provtrace/ ./qgen -s 10 -r 1755709849 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_3/provtrace/$q.traceprov.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709849 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_3/validate/$q.validate.sql; done

DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709849 11.a >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_3/validate/11.a.validate.sql
DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709849 11.b >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_3/validate/11.b.validate.sql

##########################################################################################################################################################################################

for q in `seq 1 22`; do DSS_QUERY=postgres_queries/base/ ./qgen -s 10 -r 1755709853 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_4/base/$q.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/provtrace/ ./qgen -s 10 -r 1755709853 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_4/provtrace/$q.traceprov.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709853 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_4/validate/$q.validate.sql; done

DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709853 11.a >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_4/validate/11.a.validate.sql
DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709853 11.b >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_4/validate/11.b.validate.sql


##########################################################################################################################################################################################

for q in `seq 1 22`; do DSS_QUERY=postgres_queries/base/ ./qgen -s 10 -r 1755709859 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_5/base/$q.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/provtrace/ ./qgen -s 10 -r 1755709859 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_5/provtrace/$q.traceprov.sql; done
for q in `seq 1 22`; do DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709859 $q >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_5/validate/$q.validate.sql; done

DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709859 11.a >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_5/validate/11.a.validate.sql
DSS_QUERY=postgres_queries/validate/ ./qgen -s 10 -r 1755709859 11.b >> /home/realvinayak123/projects/prov/postgres/benchmark/tpch/scale_10/params_5/validate/11.b.validate.sql

# 1755708640
# 1755708649
# 1755708657

# For SF - 10
1755709829
1755709841
1755709849
1755709853
1755709859