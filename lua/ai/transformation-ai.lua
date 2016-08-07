--[[********************************************************************
	Copyright (c) 2013-2015 Mogara

  This file is part of QSanguosha-Hegemony.

  This game is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 3.0
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  See the LICENSE file for more details.

  Mogara
*********************************************************************]]

---荀攸
local qice_skill = {}
qice_skill.name = "qice"
table.insert(sgs.ai_skills, qice_skill)
qice_skill.getTurnUseCard = function(self,room,player,data)
	if self.player:hasUsed("ViewAsSkill_qiceCard") or self.player:isKongcheng() then return end
	local handcardnum = self.player:getHandcardNum()
	local caocao = self.room:findPlayerBySkillName("jianxiong")
	local preventdamaget = false
	if not (caocao and self:isFriend(caocao)
		and caocao:getHp() > 1 
		and (caocao:hasShownSkill("jianxiong") or caocao:hasShownSkill("jjianxiong"))
		and not self:willSkipPlayPhase(caocao))
		and getCardsNum("Peach", self.player) > 0 then
		preventdamage = true
	end
	
	local usevalue = 0
	local keepvalue = 0	
	local id
	local cards = self.player:getHandcards()
	local allcard = {}
	cards = sgs.QList2Table(cards)
	for _,card in ipairs(cards) do
		if card:canRecast() then return end
		if card:isAvailable(self.player) then
			if card:isKindOf("EquipCard") and not self:getSameEquip(card) and handcardnum > 1 then
				local use = sgs.CardUseStruct(card, self.player, self.player, true)
				self:useEquipCard(card, use)
			end
			usevalue = self:getUseValue(card) + usevalue
		end
		if not id then
			id = tostring(card:getId())
		else
			id = id .. "+" .. tostring(card:getId())
		end
	end	
	self:sortByKeepValue(cards)
	for i = 1, #cards, 1 do
		if i > self:getOverflow(self.player) then
			keepvalue = self:getKeepValue(cards[i]) + keepvalue
		end
	end

	local cardsavailable = function(use_card)
		local targets = {}
		if use_card:isKindOf("AwaitExhausted") then
			for _, p in sgs.qlist(self.room:getAlivePlayers()) do
				if not self.player:isProhibited(p, use_card) and self.player:hasShownOneGeneral() and self.player:getRole() ~= "careerist" and
							p:getRole() ~= "careerist" and p:hasShownOneGeneral() and p:getKingdom() == self.player:getKingdom() then
						table.insert(targets, p)
				end
			end
		elseif use_card:getSubtype() == "global_effect" and not use_card:isKindOf("FightTogether") then
			for _, p in sgs.qlist(self.room:getAlivePlayers()) do
				if not self.player:isProhibited(p, use_card) then
					table.insert(targets, p)
				end
			end
		elseif use_card:getSubtype() == "aoe" and not use_card:isKindOf("BurningCamps") then
			for _, p in sgs.qlist(self.room:getOtherPlayers(self.player)) do
				if not self.player:isProhibited(p, use_card) then
					table.insert(targets, p)
				end
			end
		elseif use_card:isKindOf("BurningCamps") then
				players = self.player:getNextAlive():getFormation()
			for _, p in sgs.qlist(players) do
				if not self.player:isProhibited(p, use_card) then
					table.insert(targets, p)
				end
			end
		end
		return #targets <= handcardnum
	end
	
	local parsed_card = {}
	if cardsavailable(sgs.Card_Parse("amazing_grace:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("amazing_grace:qice[to_be_decided:0]=" .. id .."&qice"))		--五谷
	end
	if cardsavailable(sgs.Card_Parse("god_salvation:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("god_salvation:qice[to_be_decided:0]=" .. id .."&qice"))		--桃园
	end
	if cardsavailable(sgs.Card_Parse("burning_camps:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("burning_camps:qice[to_be_decided:0]=" .. id .."&qice"))		--火烧连营
	end
	if cardsavailable(sgs.Card_Parse("drowning:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("drowning:qice[to_be_decided:0]=" .. id .."&qice"))				--水淹七军
	end
	if cardsavailable(sgs.Card_Parse("threaten_emperor:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("threaten_emperor:qice[to_be_decided:0]=" .. id .."&qice"))		--挟天子以令诸侯
	end
	if cardsavailable(sgs.Card_Parse("await_exhausted:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("await_exhausted:qice[to_be_decided:0]=" .. id .."&qice"))			--以逸待劳
	end
	if cardsavailable(sgs.Card_Parse("befriend_attacking:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("befriend_attacking:qice[to_be_decided:0]=" .. id .."&qice"))		--远交近攻
	end
	if cardsavailable(sgs.Card_Parse("duel:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("duel:qice[to_be_decided:0]=" .. id .."&qice"))				--决斗
	end
	if cardsavailable(sgs.Card_Parse("dismantlement:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("dismantlement:qice[to_be_decided:0]=" .. id .."&qice"))		--过河拆桥
	end
	if cardsavailable(sgs.Card_Parse("snatch:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("snatch:qice[to_be_decided:0]=" .. id .."&qice"))				--顺手牵羊
	end
	if cardsavailable(sgs.Card_Parse("ex_nihilo:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("ex_nihilo:qice[to_be_decided:0]=" .. id .."&qice"))			--无中生有
	end
	if not preventdamage or not self:hasTrickEffective(sgs.Card_Parse("archery_attack:qice[to_be_decided:0]=" .. id .."&qice"), caocao)
		and cardsavailable(sgs.Card_Parse("archery_attack:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("archery_attack:qice[to_be_decided:0]=" .. id .."&qice"))	--万箭齐发
	end
	if not preventdamage or not self:hasTrickEffective(sgs.Card_Parse("savage_assault:qice[to_be_decided:0]=" .. id .."&qice"), caocao) and
		cardsavailable(sgs.Card_Parse("savage_assault:qice[to_be_decided:0]=" .. id .."&qice")) then
		table.insert(parsed_card, sgs.Card_Parse("savage_assault:qice[to_be_decided:0]=" .. id .."&qice"))	--南蛮
	end
	
	local value = 0
	local tcard
	for _, c in ipairs(parsed_card) do
		assert(c)
		if self:getUseValue(c) > value and self:getUseValue(c) > keepvalue and self:getUseValue(c) > usevalue and (usevalue < 6 or handcardnum == 1) then
			value = self:getUseValue(c)
			tcard = c
		end
	end
	if tcard then
		return tcard
	end
end

sgs.ai_skill_invoke.zhiyu = function(self, data)
	if not self:willShowForMasochism() then return false end
	local damage = data:toDamage()
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	local first
	local difcolor = 0
	for _, card in ipairs(cards)  do
		if not first then first = card end
		if (first:isRed() and card:isBlack()) or (card:isRed() and first:isBlack()) then
			difcolor = 1
			break
		end
	end
	if difcolor == 0 and damage.from then
		if self:isFriend(damage.from) and not damage.from:isKongcheng() then
			return false
		elseif self:isEnemy(damage.from) then
			if self:doNotDiscard(damage.from, "h") and not damage.from:isKongcheng() then return false end
			return true
		end
	end
	return true
end

--卞皇后
sgs.ai_skill_invoke.wanwei = function(self, data)
	if not self:willShowForDefence() then return false end
	local move = data:toMoveOneTime()
	local target = move.to
	if self:isFriend(target) then return false end
	self.wanwei = {}
	local cards = sgs.QList2Table(self.player:getCards("e"))
	local handcards = sgs.QList2Table(self.player:getHandcards())
	table.insertTable(cards, handcards)
	self:sortByUseValue(cards)
	for i = 1, move.card_ids:length(), 1 do
		table.insert(self.wanwei, card[i]:getEffectiveId())
	end
	return true
end

sgs.ai_skill_exchange["yuejian"] = function(self)
	return self.wanwei
end

sgs.ai_skill_invoke.yuejian = function(self, data)
	local target = self.room:getCurrent()
	if target:getHandcardNum() > target:getMaxCards() and target:isWounded() then return true end
	return false
end

--李郭
sgs.ai_skill_invoke.xichou = function(self, data)
	return false
end

--左慈
sgs.ai_skill_invoke.huashen = function(self, data)
	return true
end

sgs.ai_skill_invoke["xinsheng"] = function(self, data)
	if self.player:ownSkill("xichou") and not self.player:hasShownSkill("huashen") and self.player:getMark("xichou") == 0 then return false end
	return true
end

--沙摩柯
sgs.ai_skill_invoke.jili = function(self, data)
	if not self:willShowForAttack() then return false end
	return true
end

--马谡
sgs.ai_skill_invoke.zhiman = function(self, data)
	local damage = self.player:getTag("zhiman_data"):toDamage()
	local target = damage.to
	if self:isFriend(damage.to) then return true end
	if target:hasShownSkills(sgs.masochism_skill) and not target:getEquips():isEmpty() and self.player:canGetCard(target, "e") then return true end
	if self:getDangerousCard(target) and self.player:canGetCard(target, "e") then return true end
	return false
end

local sanyao_skill = {}
sanyao_skill.name = "sanyao"
table.insert(sgs.ai_skills, sanyao_skill)
sanyao_skill.getTurnUseCard = function(self,room,player,data)
	if not self:willShowForAttack() then return end
	if self.player:hasUsed("SanyaoCard") then return end
	if self.player:isNude() then return end
	return sgs.Card_Parse("@SanyaoCard=.&sanyao")
end

sgs.ai_skill_use_func.SanyaoCard = function(card, use, self)
	local targets = {}
	local maxhp = 0
    for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if maxhp < p:getHp() then p:getHp() end
	end
    for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		self.player:speak("目标是" .. p:objectName())
		if p:getHp() == maxhp then table.insert(targets, p) end
	end
	if self:isWeak() or not self.player:hasSkills(sgs.masochism_skill) then
		table.removeOne(targets, self.player)
	end
	local target
	for _, p in ipairs(targets) do
		if self:isFriend(p) and (self.player:canGetCard(p, "j") or (p:hasShownSkills(sgs.lose_equip_skill) and self.player:canGetCard(p, "e"))) then
			target = p
			break
		end
	end
	if not target then
		for _, p in ipairs(targets) do
			if self:isEnemy(p) and self:isWeak(p) then target = p break end
		end
	end
	if not target then
		for _, p in ipairs(targets) do
			if self:getDangerousCard(p) and self.player:canGetCard(p, "e") then target = p break end
		end
	end
	if not target then
		for _, p in ipairs(targets) do
			if self:isEnemy(p) and not p:hasShownSkills(sgs.masochism_skill) and self:getOverflow() > 0 then target = p break end
		end
	end

	if self:needToThrowArmor() then
		use.card = sgs.Card_Parse("@SanyaoCard=" .. self.player:getArmor():getId() .. "&sanyao")
		if #targets == 0 then use.card = nil return end
		if use.to then
			if target then
				use.to:append(target)
			else use.to:append(targets[1])
			end
			return
		end
	else
		if not target then use.card = nil return end
		local cards = {}
		for _, c in sgs.qlist(self.player:getCards("he")) do
			if c:getEffectiveId() ~= slashCard:getEffectiveId() then table.insert(cards, c) end
		end
		self:sortByKeepValue(cards)

		local card_id
		for _, c in ipairs(cards) do
			if c:isKindOf("Lightning") and not isCard("Peach", c, self.player) and not self:willUseLightning(c) then
				card_id = c:getEffectiveId()
				break
			end
		end

		if not card_id then
			for _, c in ipairs(cards) do
				if not isCard("Peach", c, self.player)
					and (c:isKindOf("AmazingGrace") or c:isKindOf("GodSalvation") and not self:willUseGodSalvation(c)) then
					card_id = c:getEffectiveId()
					break
				end
			end
		end

		if not card_id then
			for _, c in ipairs(cards) do
				if (not isCard("Peach", c, self.player) or self:getCardsNum("Peach") > 1)
					and (not isCard("Jink", c, self.player) or self:getCardsNum("Jink") > 1 or isWeak)
					and not (self.player:getWeapon() and self.player:getWeapon():getEffectiveId() == c:getEffectiveId())
					and not (self.player:getOffensiveHorse() and self.player:getOffensiveHorse():getEffectiveId() == c:getEffectiveId()) then
					card_id = c:getEffectiveId()
				end
			end
		end
		if card_id then
			use.card = sgs.Card_Parse("@SanyaoCard=" .. card_id .. "&sanyao")
			if use.to then
				if use.to:isEmpty() then use.to:append(target) return end
			end
		end
	end
	use.card = nil
end

--凌统
sgs.ai_skill_playerchosen.liefeng = function(self, targets)
	local targetlist = sgs.QList2Table(targets)
	self:sort(targetlist, "defense")
	for _, p in ipairs(targetlist) do
		if self:isFriend(p) and (self:needToThrowArmor(p) or p:hasShownSkills(sgs.lose_equip_skill)) and self.player:canDiscard(p, "e") then return p end
	end
	for _, p in ipairs(targetlist) do
		if self:isEnemy(p) and self:getDangerousCard(p) then return p end
	end
	for _, p in ipairs(targetlist) do
		if not self:isFriend(p) then return p end
	end
	return nil
end

xuanlue_skill = {}
xuanlue_skill.name = "xuanlue"
table.insert(sgs.ai_skills, xuanlue_skill)
xuanlue_skill.getTurnUseCard = function(self)
	if self.player:getMark("@lue") <= 0 then return end
	local targets = {}
	self.resultcard = {}
	self.resulttargets = {}
	self.gotcard = {}
	local danger = {}
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		p:setFlags("-xuanlue_gotweapon")
		p:setFlags("-xuanlue_gotarmor")
		p:setFlags("-xuanlue_gotoffensivehorse")
		p:setFlags("-xuanlue_gotdefensivehorse")
		p:setFlags("-xuanlue_gottreasure")
		if p:objectName() == self.player:objectName() then continue end
		if self.player:canGetCard(p, "e") then
			table.insert(targets, p)
		end
	end

	for _, p in ipairs(targets) do
		if not self:isFriend(p) then
			local dangerous = self:getDangerousCard(p)
			if dangerous and self.player:canGetCard(p, dangerous) then
				local card = sgs.Sanguosha:getCard(dangerous)
				table.insert(danger, card)
			end
		end
	end
	for _, p in ipairs(targets) do
		if not self:isFriend(p) then
			for _, eq in sgs.qlist(p:getEquips()) do
				if self.player:canGetCard(p, eq:getEffectiveId()) and not table.contains(danger, eq) then
					table.insert(danger, eq)
				end
			end
		end
	end

	for _, p in ipairs(self.friends) do
		if self:isFriend(p) and p:hasSkills(sgs.need_equip_skill .. "|" .. sgs.lose_equip_skill) then
			for _, card in ipairs(danger) do
				if (card:isKindOf("Weapon") and not p:getWeapon() and not p:hasFlag("xuanlue_gotweapon")) then
					p:setFlags("xuanlue_gotweapon")
					if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
					if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
				end
				if (card:isKindOf("Armor") and not p:getArmor() and not p:hasFlag("xuanlue_gotarmor")) then
					p:setFlags("xuanlue_gotarmor")
					if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
					if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
				end
				if (card:isKindOf("OffensiveHorse") and not p:getOffensiveHorse() and not p:hasFlag("xuanlue_gotoffensivehorse")) then
					p:setFlags("xuanlue_gotoffensivehorse")
					if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
					if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
				end
				if (card:isKindOf("DefensiveHorse") and not p:getDefensiveHorse() and not p:hasFlag("xuanlue_gotdefensivehorse")) then
					p:setFlags("xuanlue_gotdefensivehorse")
					if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
					if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
				end
				if (card:isKindOf("Treasure") and not p:getTreasure() and not p:hasFlag("xuanlue_gottreasure")) then
					p:setFlags("xuanlue_gottreasure")
					if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
					if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
				end
				if #self.resultcard >= #danger or #self.resultcard >= 3 then break end
			end
			for _, card in ipairs(self.resultcard) do
				table.removeOne(danger, card)
			end
		end
		if #self.resultcard >= #danger or #self.resultcard >= 3 then break end
	end

	if #self.resultcard < 3 and #self.resultcard < #danger then
		for _, p in ipairs(self.friends) do
			if self:isFriend(p) then
				for _, card in ipairs(danger) do
					if (card:isKindOf("Weapon") and not p:getWeapon() and not p:hasFlag("xuanlue_gotweapon")) then
						p:setFlags("xuanlue_gotweapon")
						if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
						if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
					end
					if (card:isKindOf("Armor") and not p:getArmor() and not p:hasFlag("xuanlue_gotarmor")) then
						p:setFlags("xuanlue_gotarmor")
						if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
						if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
					end
					if (card:isKindOf("OffensiveHorse") and not p:getOffensiveHorse() and not p:hasFlag("xuanlue_gotoffensivehorse")) then
						p:setFlags("xuanlue_gotoffensivehorse")
						if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
						if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
					end
					if (card:isKindOf("DefensiveHorse") and not p:getDefensiveHorse() and not p:hasFlag("xuanlue_gotdefensivehorse")) then
						p:setFlags("xuanlue_gotdefensivehorse")
						if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
						if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
					end
					if (card:isKindOf("Treasure") and not p:getTreasure() and not p:hasFlag("xuanlue_gottreasure")) then
						p:setFlags("xuanlue_gottreasure")
						if not table.contains(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) then table.insert(self.resulttargets, self.room:getCardOwner(card:getEffectiveId())) end
						if not table.contains(self.resultcard, card) then table.insert(self.resultcard, card) end
					end
					if #self.resultcard >= #danger or #self.resultcard >= 3 then break end
				end
				for _, card in ipairs(self.resultcard) do
					table.removeOne(danger, card)
				end
			end
			if #self.resultcard >= #danger or #self.resultcard >= 3 then break end
		end
	end

	if #self.resultcard >= 3 then
		sgs.ai_use_priority.XuanlueCard = 9
		return sgs.Card_Parse("@XuanlueCard=.&xuanlue")
	else
		for _, p in ipairs(self.friends_noself) do
			if self:needToThrowArmor(p) and self.player:canGetCard(p, p:getArmor():getEffectiveId()) then
				if not table.contains(self.resultcard, p:getArmor()) then table.insert(self.resultcard, p:getArmor()) end
				if not table.contains(self.resulttargets, p) then table.insert(self.resulttargets, p) end
			end
			if p:hasShownSkills(sgs.lose_equip_skill) and p:hasEquip() then
				for _, eq in sgs.qlist(p:getEquips()) do
					if self.player:canGetCard(p, eq:getEffectiveId()) then
						if not table.contains(self.resultcard, eq) then table.insert(self.resultcard, eq) end
						if not table.contains(self.resulttargets, p) then table.insert(self.resulttargets, p) end
					end
				end
			end
		end
	end
	if #self.resultcard >= 3 then
		sgs.ai_use_priority.XuanlueCard = 9
		return sgs.Card_Parse("@XuanlueCard=.&xuanlue")
	else return end
end

sgs.ai_skill_use_func.XuanlueCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_playerchosen.xuanlue = function(self, targets)
	local targetlist = sgs.QList2Table(targets)
	for _, p in ipairs(targetlist) do
		if not self:isFriend(p) and table.contains(self.resulttargets, p) then
			for _, eq in sgs.qlist(p:getEquips()) do
				if table.contains(self.resultcard, eq) and not table.contains(self.gotcard, eq) then
					return p
				end
			end
		end
	end
	for _, p in ipairs(targetlist) do
		if table.contains(self.resulttargets, p) then
			for _, eq in sgs.qlist(p:getEquips()) do
				if table.contains(self.resultcard, eq) and not table.contains(self.gotcard, eq) then
					return p
				end
			end
		end
	end
end

sgs.ai_skill_cardchosen.xuanlue = function(self, p)
	for _, eq in sgs.qlist(p:getEquips()) do
		if table.contains(self.resultcard, eq) and not table.contains(self.gotcard, eq) then
			table.insert(self.gotcard, eq)
			return eq:getEffectiveId()
		end
	end
end

sgs.ai_skill_use["@@xuanlue_equip!"] = function(self, prompt)
	local card_ids = self.player:property("xuanlue_cards"):toString():split("+")
	for _, id in ipairs(card_ids) do
		local card = sgs.Sanguosha:getCard(id)
		for _, p in ipairs(self.friends) do
			if card:isKindOf("Weapon") and p:hasFlag("xuanlue_gotweapon") then
				p:setFlags("-xuanlue_gotweapon")
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("Armor") and p:hasFlag("xuanlue_gotarmor") then
				p:setFlags("-xuanlue_gotarmor")
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("OffensiveHorse") and p:hasFlag("xuanlue_gotoffensivehorse") then
				p:setFlags("-xuanlue_gotoffensivehorse")
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("DefensiveHorse") and p:hasFlag("xuanlue_gotdefensivehorse") then
				p:setFlags("-xuanlue_gotdefensivehorse")
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("Treasure") and p:hasFlag("xuanlue_gottreasure") then
				p:setFlags("-xuanlue_gottreasure")
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
		end
	end
	for _, id in ipairs(card_ids) do
		local card = sgs.Sanguosha:getCard(id)
		for _, p in ipairs(self.friends) do
			if card:isKindOf("Weapon") and not p:getWeapon() then
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("Armor") and not p:getArmor() then
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("OffensiveHorse") and not p:getOffensiveHorse() then
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("DefensiveHorse") and not p:getDefensiveHorse() then
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
			if card:isKindOf("Treasure") and not p:getTreasure() then
				return ("@XuanlueequipCard=" .. id .. "&xuanlueequip->" .. p:objectName())
			end
		end
	end
	return "."
end

--吕范
local diaodu_skill = {}
diaodu_skill.name = "diaodu"
table.insert(sgs.ai_skills, diaodu_skill)
diaodu_skill.getTurnUseCard = function(self,room,player,data)
	if self.player:hasUsed("DiaoduCard") then return end
	if #self.friends_noself == 0 then return end
	return sgs.Card_Parse("@DiaoduCard=.&diaodu")
end

sgs.ai_skill_use_func.DiaoduCard = function(card, use, self)
	use.card = nil
	for _, card in sgs.qlist(self.player:getCards("h")) do
		if card:isKindOf("Weapon") and not self.player:getWeapon() then
			use.card = card
		end
		if card:isKindOf("Armor") and not self.player:getArmor() then
			use.card = card
		end
		if card:isKindOf("OffensiveHorse") and not self.player:getOffensiveHorse() then
			use.card = card
		end
		if card:isKindOf("DefensiveHorse") and not self.player:getDefensiveHorse() then
			use.card = card
		end
		if card:isKindOf("Treasure") and not self.player:getTreasure() then
			use.card = card
		end
		if use.card then return end
	end
	use.card = card
end

sgs.ai_skill_use["@@diaodu_equip"] = function(self, prompt)
	local id

	if self:needToThrowArmor() then
		for _, p in ipairs(self.friends_noself) do
			if self.player:isFriendWith(p) and not p:getArmor() then
				return ("@DiaoduequipCard=" .. self.player:getArmor():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
		end
	end
	for _, p in ipairs(self.friends_noself) do
		if not self.player:isFriendWith(p) then continue end
		for _, card in sgs.qlist(self.player:getCards("h")) do
			if card:isKindOf("Weapon") and self.player:getWeapon() then
				return "@DiaoduequipCard=" .. self.player:getWeapon():getEffectiveId() .. "&diaoduequip->" .. p:objectName()
			end
			if card:isKindOf("Armor") and self.player:getArmor() and not (self.player:getArmor():isKindOf("PeaceSpell") and self:isWeak()) then
				return "@DiaoduequipCard=" .. self.player:getArmor():getEffectiveId() .. "&diaoduequip->" .. p:objectName()
			end
			if card:isKindOf("OffensiveHorse") and self.player:getOffensiveHorse() then
				return "@DiaoduequipCard=" .. self.player:getOffensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName()
			end
			if card:isKindOf("DefensiveHorse") and self.player:getDefensiveHorse() then
				return "@DiaoduequipCard=" .. self.player:getDefensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName()
			end
			if card:isKindOf("Treasure") and self.player:getTreasure() and (not self.player:getArmor():isKindOf("WoodenOx") and self.player:getPile("wooden_ox"):length() > 1) then
				return "@DiaoduequipCard=" .. self.player:getTreasure():getEffectiveId() .. "&diaoduequip->" .. p:objectName()
			end
		end
	end
	if self.player:hasSkills(sgs.lose_equip_skill) then
		for _, card in sgs.qlist(self.player:getCards("e")) do
			for _, p in ipairs(self.friends_noself) do
				if not self.player:isFriendWith(p) then continue end
				if card:isKindOf("Armor") and not p:getArmor() and not(card:isKindOf("PeaceSpell") and self:isWeak()) then
					return ("@DiaoduequipCard=" .. self.player:getArmor():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
				end
				if card:isKindOf("OffensiveHorse") and not p:getOffensiveHorse() then
					return ("@DiaoduequipCard=" .. self.player:getOffensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
				end
				if card:isKindOf("DefensiveHorse") and not p:getDefensiveHorse() then
					return ("@DiaoduequipCard=" .. self.player:getDefensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
				end
				if card:isKindOf("Treasure") and not p:getTreasure() and (not self.player:getArmor():isKindOf("WoodenOx") and self.player:getPile("wooden_ox"):length() > 1) then
					return ("@DiaoduequipCard=" .. self.player:getTreasure():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
				end
				if card:isKindOf("Weapon") and not p:getWeapon() then
					return ("@DiaoduequipCard=" .. self.player:getWeapon():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
				end
			end
		end
	end
	for _, card in sgs.qlist(self.player:getCards("e")) do
		for _, p in ipairs(self.friends_noself) do
			if not self.player:isFriendWith(p) or not p:hasShownSkills(sgs.need_equip_skill .. "|" .. sgs.lose_equip_skill) then continue end
			if card:isKindOf("Armor") and not p:getArmor() and not(card:isKindOf("PeaceSpell") and self:isWeak()) then
				return ("@DiaoduequipCard=" .. self.player:getArmor():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
			if card:isKindOf("OffensiveHorse") and not p:getOffensiveHorse() then
				return ("@DiaoduequipCard=" .. self.player:getOffensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
			if card:isKindOf("DefensiveHorse") and not p:getDefensiveHorse() then
				return ("@DiaoduequipCard=" .. self.player:getDefensiveHorse():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
			if card:isKindOf("Treasure") and not p:getTreasure() and (not self.player:getArmor():isKindOf("WoodenOx") and self.player:getPile("wooden_ox"):length() > 1) then
				return ("@DiaoduequipCard=" .. self.player:getTreasure():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
			if card:isKindOf("Weapon") and not p:getWeapon() then
				return ("@DiaoduequipCard=" .. self.player:getWeapon():getEffectiveId() .. "&diaoduequip->" .. p:objectName())
			end
		end
	end

	for _, card in sgs.qlist(self.player:getCards("h")) do
		if card:isKindOf("EquipCard") then
			local dummy_use = {isDummy = true}
			self:useEquipCard(card, dummy_use)
			if dummy_use.card and dummy_use.card:getEffectiveId() == card:getEffectiveId() then
				return card:toString()
			end
		end
	end
	return "."
end

sgs.ai_skill_invoke.diancai = function(self, data)
	if not self:willShowForDefence() then return false end
	if self.player:getHandcardNum() < self.player:getMaxHp() then return true end
	return false
end

--孙权
local lianzi_skill = {}
lianzi_skill.name = "lianzi"
table.insert(sgs.ai_skills, lianzi_skill)
lianzi_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("LianziCard") or self.player:isKongcheng() then return end
	local num = 0
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:hasShownOneGeneral() and p:getKingdom() == "wu" and p:getRole() ~= "careerist" then
			num = num + p:getEquips():length()
		end
	end
	if num >= 3 then
		local handcards = sgs.QList2Table(self.player:getHandcards())
		table.insertTable(cards, handcards)
		self:sortByUseValue(cards)
		return sgs.Card_Parse("@LianziCard=" .. cards[1]:getEffectiveId() .. "&lianzi")
	end
end

sgs.ai_skill_use_func.LianziCard = function(card, use, self)
	use.card = card
end

sgs.ai_skill_movecards.lianzi = function(self, upcards, downcards, min_num, max_num)
	local ups = {}
	for _, id in ipairs(upcards) do
		if sgs.Sanguosha:getCard(id):hasFlag("lianzi") then
			table.insert(downcards, id)
		else
			table.insert(ups, id)
		end
	end
	return ups, downcards
end

sgs.ai_skill_invoke.jubao = function(self, data)
	return true
end

--夜明珠
local Luminouspearl_skill = {}
Luminouspearl_skill.name = "Luminouspearl"
table.insert(sgs.ai_skills, Luminouspearl_skill)
Luminouspearl_skill.getTurnUseCard = function(self)
	if not self.player:hasUsed("LuminouspearlCard") and not self.player:hasShownSkill("zhiheng") and self.player:hasTreasure("Luminouspearl") then
		return sgs.Card_Parse("@LuminouspearlCard=.")
	end
end

sgs.ai_skill_use_func.LuminouspearlCard = function(card, use, self)
	local unpreferedCards = {}
	local cards = sgs.QList2Table(self.player:getHandcards())

	if self:getCardsNum("Crossbow", 'he') > 0 and #self.enemies > 0 and self.player:getCardCount(true) >= 4 then
		local zcards = sgs.QList2Table(self.player:getCards("he"))
		table.removeOne(zcards, self.player:getTreasure())
		self:sortByUseValue(zcards, true)
		for _, zcard in ipairs(zcards) do
			if not isCard("Peach", zcard, self.player) and (self.player:getOffensiveHorse() or card:isKindOf("OffensiveHorse")) and not self.player:isJilei(zcard) then
				table.insert(unpreferedCards, zcard:getEffectiveId())
				if #unpreferedCards >= self.player:getMaxHp() then break end
			end
		end
		self.player:speak("准备用掉：" .. #unpreferedCards)
		if #unpreferedCards > 0 then
			use.card = sgs.Card_Parse("@LuminouspearlCard=" .. table.concat(unpreferedCards, "+"))
			return
		end
	end

	if self.player:getHp() < 3 then
		local zcards = self.player:getCards("he")
		local use_slash, keep_jink, keep_analeptic, keep_weapon = false, false, false
		local zcards = sgs.QList2Table(self.player:getCards("he"))
		table.removeOne(zcards, self.player:getTreasure())
		self:sortByUseValue(zcards, true)
		for _, zcard in ipairs(zcards) do
			if not isCard("Peach", zcard, self.player) and not isCard("ExNihilo", zcard, self.player) then
				local shouldUse = true
				if isCard("Slash", zcard, self.player) and not use_slash then
					local dummy_use = { isDummy = true , to = sgs.SPlayerList()}
					self:useBasicCard(zcard, dummy_use)
					if dummy_use.card then
						if dummy_use.to then
							for _, p in sgs.qlist(dummy_use.to) do
								if p:getHp() <= 1 then
									shouldUse = false
									if self.player:distanceTo(p) > 1 then keep_weapon = self.player:getWeapon() end
									break
								end
							end
							if dummy_use.to:length() > 1 then shouldUse = false end
						end
						if not self:isWeak() then shouldUse = false end
						if not shouldUse then use_slash = true end
					end
				end
				if zcard:getTypeId() == sgs.Card_TypeTrick then
					local dummy_use = { isDummy = true }
					self:useTrickCard(zcard, dummy_use)
					if dummy_use.card then shouldUse = false end
				end
				if zcard:getTypeId() == sgs.Card_TypeEquip and not self.player:hasEquip(zcard) then
					local dummy_use = { isDummy = true }
					self:useEquipCard(zcard, dummy_use)
					if dummy_use.card then shouldUse = false end
					if keep_weapon and zcard:getEffectiveId() == keep_weapon:getEffectiveId() then shouldUse = false end
				end
				if self.player:hasEquip(zcard) and zcard:isKindOf("Armor") and not self:needToThrowArmor() then shouldUse = false end
				if self.player:hasEquip(zcard) and zcard:isKindOf("DefensiveHorse") and not self:needToThrowArmor() then shouldUse = false end
				if isCard("Jink", zcard, self.player) and not keep_jink then
					keep_jink = true
					shouldUse = false
				end
				if self.player:getHp() == 1 and isCard("Analeptic", zcard, self.player) and not keep_analeptic then
					keep_analeptic = true
					shouldUse = false
				end
				if shouldUse then table.insert(unpreferedCards, zcard:getId()) end
			end
		end
	end

	if #unpreferedCards == 0 then
		local use_slash_num = 0
		self:sortByKeepValue(cards)
		for _, card in ipairs(cards) do
			if card:isKindOf("Slash") then
				local will_use = false
				if use_slash_num <= sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, card) then
					local dummy_use = { isDummy = true }
					self:useBasicCard(card, dummy_use)
					if dummy_use.card then
						will_use = true
						use_slash_num = use_slash_num + 1
					end
				end
				if not will_use then table.insert(unpreferedCards, card:getId()) end
			end
		end

		local num = self:getCardsNum("Jink") - 1
		if self.player:getArmor() then num = num + 1 end
		if num > 0 then
			for _, card in ipairs(cards) do
				if card:isKindOf("Jink") and num > 0 then
					table.insert(unpreferedCards, card:getId())
					num = num - 1
				end
			end
		end
		for _, card in ipairs(cards) do
			if (card:isKindOf("Weapon") and self.player:getHandcardNum() < 3) or card:isKindOf("OffensiveHorse")
				or self:getSameEquip(card, self.player) or card:isKindOf("AmazingGrace") then
				table.insert(unpreferedCards, card:getId())
			elseif card:getTypeId() == sgs.Card_TypeTrick then
				local dummy_use = { isDummy = true }
				self:useTrickCard(card, dummy_use)
				if not dummy_use.card then table.insert(unpreferedCards, card:getId()) end
			end
		end

		if self.player:getWeapon() and self.player:getHandcardNum() < 3 then
			table.insert(unpreferedCards, self.player:getWeapon():getId())
		end

		if self:needToThrowArmor() then
			table.insert(unpreferedCards, self.player:getArmor():getId())
		end

		if self.player:getOffensiveHorse() and self.player:getWeapon() then
			table.insert(unpreferedCards, self.player:getOffensiveHorse():getId())
		end

	end
	
	for index = #unpreferedCards, 1, -1 do
		if sgs.Sanguosha:getCard(unpreferedCards[index]):isKindOf("WoodenOx") and self.player:getPile("wooden_ox"):length() > 1 then
			table.removeOne(unpreferedCards, unpreferedCards[index])
		end
	end
	
	local has_equip = {}
	if self.player:hasSkills(sgs.lose_equip_skill) then
		for index = #unpreferedCards, 1, -1 do
			if self.player:hasEquip(sgs.Sanguosha:getCard(unpreferedCards[index])) then
				table.insert(has_equip, unpreferedCards[index])
				if #has_equip > 1 then
					table.removeOne(unpreferedCards, unpreferedCards[index])
				end
			end	
		end
	end	
	
	local use_cards = {}
	for index = #unpreferedCards, 1, -1 do
		if not self.player:isJilei(sgs.Sanguosha:getCard(unpreferedCards[index])) then
			if #use_cards < self.player:getMaxHp() then
				table.insert(use_cards, unpreferedCards[index])
			end
		end
	end
	self.player:speak("有废牌：" .. #use_cards)
	if #use_cards > 0 then
		use.card = sgs.Card_Parse("@LuminouspearlCard=" .. table.concat(use_cards, "+"))
	end
end

sgs.ai_use_priority.Luminouspearl = 7
sgs.ai_keep_value.Luminouspearl = 4

sgs.ai_use_value.LuminouspearlCard = 9
sgs.ai_use_priority.LuminouspearlCard = 2.61
sgs.dynamic_value.benefit.LuminouspearlCard = true