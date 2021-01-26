
#include "interchange.hpp"


using namespace eosio;


[[eosio::action]] void interchange::cancel(eosio::name username, uint64_t id, uint64_t est_converted_month){
  require_auth(_changer);

  changes_index changes(_me, _me.value);
  auto change = changes.find(id);

  eosio::check(change != changes.end(), "Change object is not found");
  eosio::check(change -> status == "process"_n, "Change object is proccessed or canceled");

  if (change -> is_lock == false) {
    
    action(
        permission_level{ _me, "active"_n },
        _token_contract, "transfer"_n,
        std::make_tuple( _me, change -> username, change -> cru_quantity, std::string("")) 
    ).send(); 

  } else {

    action(
        permission_level{ _me, "active"_n },
        _tokenlock, "intcancel"_n,
        std::make_tuple( change -> username, change -> cru_quantity, change->lock_id, est_converted_month) 
    ).send();

    action(
        permission_level{ _me, "active"_n },
        _token_contract, "transfer"_n,
        std::make_tuple( _me, _tokenlock, change -> cru_quantity, std::string("")) 
    ).send(); 

  }

  changes.erase(change);  

}

[[eosio::action]] void interchange::intchange(eosio::name username, eosio::asset quantity, uint64_t lock_id){

  require_auth(_tokenlock);

  interchange::create_change_object(username, quantity, true, lock_id);

};


[[eosio::action]] void interchange::approvech(uint64_t id, eosio::name username, uint64_t lock_id, eosio::asset amount){
    require_auth(_changer);

    changes_index changes(_me, _me.value);
    auto change = changes.find(id);

    eosio::check(change != changes.end(), "Change object is not found");
    eosio::check(change -> status == "process"_n, "Change object is proccessed or canceled");
    eosio::check(change -> username == username, "Wrong username for current change");
    eosio::check(change -> wcru_quantity == amount, "Wrong amount for approve");

    // action(
    //     permission_level{ _me, "active"_n },
    //     _token_contract, "retire"_n,
    //     std::make_tuple( _me, change -> cru_quantity, std::string("retire")) 
    // ).send(); 

    eosio::check(lock_id != 0, "lock_id is should be setted");
    
    changes.modify(change, _changer, [&](auto &c){
      c.lock_id = lock_id;
      c.status = "success"_n;
    });

    //TODO method ADD on tokenlock by algorithm 3
    auto dt = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());

    action(
        permission_level{ _tokenlock, "active"_n },
        _tokenlock, "add"_n,
        std::make_tuple( change->username, (uint64_t)lock_id, (uint64_t)0, dt, (uint64_t)3, (int64_t)change->wcru_quantity.amount) 
    ).send(); 
    

  }

 void interchange::create_change_object(eosio::name username, eosio::asset quantity, bool is_lock, uint64_t lock_id){
    
    changes_index changes(_me, _me.value);
    eosio::check(quantity.symbol == _cru_symbol, "Wrong symbol for change");

    rate_index rates(_me, _me.value);
    auto rate = rates.find(0);
    double exchange_rate = 1;

    if (rate != rates.end())
      exchange_rate = rate -> exchange_rate;

    changes.emplace(_me, [&](auto &r){
      r.id = changes.available_primary_key();
      r.date = eosio::time_point_sec(eosio::current_time_point().sec_since_epoch());;
      r.username = username;
      r.cru_quantity = quantity;
      r.wcru_quantity = eosio::asset(quantity.amount, _wcru_symbol);
      r.status = "process"_n;
      r.is_lock = is_lock;
      r.lock_id = lock_id;
      r.exchange_rate = exchange_rate;
    });
    
  }


extern "C" {
   
   /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if (code == interchange::_me.value) {
          if (action == "approvech"_n.value){
            execute_action(name(receiver), name(code), &interchange::approvech);
          }  else if (action == "intchange"_n.value){
            execute_action(name(receiver), name(code), &interchange::intchange);
          } else if (action == "cancel"_n.value){
            execute_action(name(receiver), name(code), &interchange::cancel);
          }               
        } else {

          if (action == "transfer"_n.value){
            
            struct transfer {
                eosio::name from;
                eosio::name to;
                eosio::asset quantity;
                std::string memo;
            };

            auto op = eosio::unpack_action_data<transfer>();

            if (op.to == interchange::_me){
                if (op.from != interchange::_worldcru && op.from != interchange::_tokenlock){
                  require_auth(op.from);
                  eosio::check(name(code) == interchange::_token_contract, "Wrong token contract");
                  eosio::check(op.quantity.symbol == interchange::_cru_symbol, "Wrong symbol");

                  interchange::create_change_object(op.from, op.quantity, false, 0);  
                }
                
            }
          }
        }
  };
};
